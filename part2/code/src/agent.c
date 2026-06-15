// Агент-наблюдатель: оркестрация главного цикла мониторинга.
#include "agent.h"
#include "ptrace_util.h"
#include "guard.h"
#include "inject.h"
#include "attrib.h"
#include "event.h"

#include <sys/user.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAX_MAPS 1024

// Разрешён ли инициатор: имя функции присутствует в белом списке.
static int func_allowed(const agent_config_t *cfg, const char *func) {
    if (cfg->allow_count == 0) return 0;   // список пуст — никто не разрешён
    if (!func || func[0] == '\0') return 0;
    for (int i = 0; i < cfg->allow_count; i++)
        if (strcmp(cfg->allow_funcs[i], func) == 0)
            return 1;
    return 0;
}

int agent_run(const agent_config_t *cfg) {
    stop_info_t st;
    int exit_code = 0;

    // 1) запустить цель под трассировкой
    pid_t pid = pt_launch(cfg->target_argv);
    if (pid < 0) return 1;

    // 2) первая остановка — на execve целевой программы
    if (pt_wait(pid, &st) < 0) return 1;
    if (st.reason != STOP_SIGNAL) {
        fprintf(stderr, "agent: цель не остановилась на старте\n");
        return 1;
    }

    // 3) найти инструкцию syscall для инжекции mprotect
    uintptr_t syscall_insn = inject_find_syscall_insn(pid);
    if (!syscall_insn) {
        fprintf(stderr, "agent: не найдена инструкция syscall в цели\n");
        return 1;
    }

    // 4) разрешить охраняемую область
    guard_region_t region;
    if (guard_resolve(pid, &cfg->guard, &region) < 0) {
        fprintf(stderr, "agent: не удалось разрешить охраняемую область\n");
        return 1;
    }
    fprintf(stderr, "agent: охрана области 0x%lx (%zu байт), страницы 0x%lx..+0x%zx\n",
            (unsigned long)region.addr, region.length,
            (unsigned long)region.page_addr, region.page_span);

    // 5) установить охрану — снять право записи
    if (guard_protect(pid, syscall_insn, &region) < 0) {
        fprintf(stderr, "agent: не удалось установить охрану\n");
        return 1;
    }

    FILE *log = event_log_open(cfg->log_path);
    uint64_t seq = 0;

    // 6) главный цикл
    for (;;) {
        if (pt_continue(pid, 0) < 0) break;
        if (pt_wait(pid, &st) < 0) break;

        if (st.reason == STOP_EXITED) {
            fprintf(stderr, "agent: цель завершилась с кодом %d\n", st.exit_code);
            exit_code = st.exit_code;
            break;
        }
        if (st.reason == STOP_SIGNALED) {
            fprintf(stderr, "agent: цель убита сигналом %d\n", st.signal);
            break;
        }
        if (st.reason != STOP_SIGNAL) break;

        if (st.signal == SIGSEGV) {
            uintptr_t fault = 0;
            if (pt_get_siginfo(pid, &fault) < 0) break;

            int in_object = guard_contains(&region, fault);
            int on_guarded_page = (fault >= region.page_addr &&
                                   fault <  region.page_addr + region.page_span);

            if (in_object || on_guarded_page) {
                // Нарушение на охраняемой странице. Если попало в сам объект —
                // регистрируем событие; если в соседние данные той же страницы —
                // это следствие страничной гранулярности mprotect, событие не
                // регистрируем, но обращение обязаны пропустить (иначе цель
                // зациклится на нём). В обоих случаях пропускаем через
                // "снять права -> шаг -> вернуть охрану".
                if (in_object) {
                    struct user_regs_struct regs;
                    if (pt_get_regs(pid, &regs) < 0) break;

                    map_entry_t maps[MAX_MAPS];
                    int n = attrib_read_maps(pid, maps, MAX_MAPS);

                    event_t ev;
                    memset(&ev, 0, sizeof(ev));
                    ev.tid = pid;             // однопоточная цель: TID == PID
                    ev.fault_addr = fault;
                    ev.rip = (uintptr_t)regs.rip;
                    ev.access_type = 'W';
                    if (n > 0)
                        attrib_resolve_addr(maps, n, ev.rip, &ev.attrib);

                    // Различение по инициатору: запись из функции белого списка
                    // считается легитимной и не регистрируется как нарушение.
                    if (!func_allowed(cfg, ev.attrib.function)) {
                        ev.seq = ++seq;
                        event_log_write(log, &ev);
                    }
                }

                // пропустить обращение: вернуть права -> один шаг -> снять снова
                if (guard_unprotect(pid, syscall_insn, &region) < 0) break;
                if (pt_singlestep(pid, 0) < 0) break;
                if (pt_wait(pid, &st) < 0) break;
                if (st.reason == STOP_EXITED) { exit_code = st.exit_code; break; }
                if (st.reason == STOP_SIGNALED) break;
                if (guard_protect(pid, syscall_insn, &region) < 0) break;
            } else {
                // настоящий чужой сегфолт вне охраняемой страницы — пробросить
                if (pt_continue(pid, SIGSEGV) < 0) break;
                if (pt_wait(pid, &st) < 0) break;
                if (st.reason == STOP_EXITED) { exit_code = st.exit_code; break; }
                if (st.reason == STOP_SIGNALED) break;
            }
        } else if (st.signal == SIGTRAP) {
            // служебная остановка — продолжаем
            continue;
        } else {
            // прочие сигналы — пробрасываем цели
            if (pt_continue(pid, st.signal) < 0) break;
            if (pt_wait(pid, &st) < 0) break;
            if (st.reason == STOP_EXITED) { exit_code = st.exit_code; break; }
        }
    }

    event_log_close(log);
    return exit_code;
}
