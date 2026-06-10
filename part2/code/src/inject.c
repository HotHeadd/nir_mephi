/*
 * Инжекция системного вызова mprotect в адресное пространство цели.
 *
 * Внешний агент не может изменить права чужого адресного пространства напрямую:
 * mprotect действует на вызывающий процесс. Поэтому агент заставляет САМУ цель
 * исполнить mprotect, временно подменив её регистры и указатель инструкций.
 */
#include "inject.h"
#include "ptrace_util.h"

#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

/*
 * Ищем готовую инструкцию syscall (опкод 0f 05) в исполнимом отображении цели.
 * Это позволяет не записывать собственный код в чужую память: мы лишь
 * указываем rip на уже существующую инструкцию.
 *
 * Сканируем первый исполнимый регион из /proc/<pid>/maps. Для надёжности
 * читаем регион через /proc/<pid>/mem.
 */
uintptr_t inject_find_syscall_insn(pid_t pid) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", (int)pid);
    FILE *f = fopen(maps_path, "r");
    if (!f) { perror("fopen maps"); return 0; }

    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", (int)pid);
    FILE *mem = fopen(mem_path, "rb");
    if (!mem) { perror("fopen mem"); fclose(f); return 0; }

    char line[512];
    uintptr_t found = 0;
    while (!found && fgets(line, sizeof(line), f)) {
        uintptr_t start, end;
        char perms[5] = {0};
        /* формат: start-end perms offset dev inode path */
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s", &start, &end, perms) != 3)
            continue;
        if (perms[2] != 'x')   /* нужен исполнимый регион */
            continue;

        size_t span = end - start;
        if (span > (size_t)4 * 1024 * 1024)  /* ограничим скан 4 МиБ для скорости */
            span = 4 * 1024 * 1024;

        unsigned char *buf = malloc(span);
        if (!buf) break;

        if (fseeko(mem, (off_t)start, SEEK_SET) == 0 &&
            fread(buf, 1, span, mem) >= 2) {
            for (size_t i = 0; i + 1 < span; i++) {
                if (buf[i] == 0x0f && buf[i + 1] == 0x05) {
                    found = start + i;
                    break;
                }
            }
        }
        free(buf);
    }

    fclose(mem);
    fclose(f);
    return found;
}

int inject_mprotect(pid_t pid, uintptr_t syscall_insn,
                    uintptr_t addr, size_t len, int prot) {
    struct user_regs_struct saved, work;
    stop_info_t st;

    if (syscall_insn == 0) {
        fprintf(stderr, "inject_mprotect: нет адреса инструкции syscall\n");
        return -1;
    }

    /* 1) сохранить регистры цели */
    if (pt_get_regs(pid, &saved) < 0) return -1;

    /* 2) подготовить регистры для вызова mprotect(addr, len, prot) */
    work = saved;
    work.rip = syscall_insn;
    work.rax = __NR_mprotect;   /* номер системного вызова */
    work.rdi = addr;            /* 1-й аргумент: адрес */
    work.rsi = len;             /* 2-й аргумент: длина */
    work.rdx = (unsigned long)prot; /* 3-й аргумент: права */

    if (pt_set_regs(pid, &work) < 0) return -1;

    /* 3) исполнить одну инструкцию — это и будет syscall */
    if (pt_singlestep(pid, 0) < 0) return -1;
    if (pt_wait(pid, &st) < 0) return -1;
    if (st.reason != STOP_SIGNAL) {
        fprintf(stderr, "inject_mprotect: неожиданная остановка после syscall\n");
        return -1;
    }

    /* 4) прочитать результат mprotect из rax */
    if (pt_get_regs(pid, &work) < 0) return -1;
    long ret = (long)work.rax;

    /* 5) восстановить исходные регистры цели */
    if (pt_set_regs(pid, &saved) < 0) return -1;

    if (ret != 0) {
        fprintf(stderr, "inject_mprotect: mprotect вернул %ld\n", ret);
        return -1;
    }
    return 0;
}
