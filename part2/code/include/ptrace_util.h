// Тонкие обёртки над примитивами ptrace и waitpid.
#pragma once

#include <sys/types.h>
#include <sys/user.h>
#include <stdint.h>
#include <stdbool.h>

// Причина остановки отслеживаемого процесса.
typedef enum {
    STOP_NONE = 0,
    STOP_SIGNAL,        // доставлен сигнал (в т.ч. SIGSEGV)
    STOP_EXITED,        // цель завершилась
    STOP_SIGNALED,      // цель убита сигналом
    STOP_UNKNOWN
} stop_reason_t;

typedef struct {
    stop_reason_t reason;
    int           signal;
    int           exit_code;
} stop_info_t;

// Запуск целевой программы под трассировкой (fork + TRACEME + execve).
pid_t pt_launch(char **argv);

// Ожидание следующей остановки цели; заполняет info.
int pt_wait(pid_t pid, stop_info_t *info);

// Продолжение исполнения цели; sig — доставляемый сигнал (0 — без).
int pt_continue(pid_t pid, int sig);

// Одиночный шаг.
int pt_singlestep(pid_t pid, int sig);

// Чтение/запись общих регистров цели.
int pt_get_regs(pid_t pid, struct user_regs_struct *regs);
int pt_set_regs(pid_t pid, const struct user_regs_struct *regs);

// Адрес обращения, вызвавшего нарушение (si_addr из siginfo).
int pt_get_siginfo(pid_t pid, uintptr_t *fault_addr);
