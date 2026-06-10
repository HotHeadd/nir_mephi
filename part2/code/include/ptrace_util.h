/*
 * Тонкие обёртки над примитивами ptrace и waitpid.
 * Изолируют системные вызовы перехвата от логики агента.
 */
#ifndef PTRACE_UTIL_H
#define PTRACE_UTIL_H

#include <sys/types.h>
#include <sys/user.h>
#include <stdint.h>
#include <stdbool.h>

/* Причина остановки отслеживаемого процесса. */
typedef enum {
    STOP_NONE = 0,
    STOP_SIGNAL,        /* доставлен сигнал (в т.ч. SIGSEGV — наш случай) */
    STOP_EXITED,        /* цель завершилась */
    STOP_SIGNALED,      /* цель убита сигналом */
    STOP_UNKNOWN
} stop_reason_t;

typedef struct {
    stop_reason_t reason;
    int           signal;    /* номер сигнала при STOP_SIGNAL/STOP_SIGNALED */
    int           exit_code; /* код выхода при STOP_EXITED */
} stop_info_t;

/* TODO: запустить целевую программу под трассировкой (fork + TRACEME + execve).
 * Возвращает pid цели или -1. */
pid_t pt_launch(char **argv);

/* TODO: дождаться следующей остановки цели; заполнить info. */
int pt_wait(pid_t pid, stop_info_t *info);

/* TODO: продолжить исполнение цели (PTRACE_CONT), доставив сигнал sig (0 — без). */
int pt_continue(pid_t pid, int sig);

/* TODO: одиночный шаг (PTRACE_SINGLESTEP). */
int pt_singlestep(pid_t pid, int sig);

/* TODO: прочитать общие регистры (PTRACE_GETREGS/GETREGSET). */
int pt_get_regs(pid_t pid, struct user_regs_struct *regs);

/* TODO: записать общие регистры (PTRACE_SETREGS/SETREGSET). */
int pt_set_regs(pid_t pid, const struct user_regs_struct *regs);

/* TODO: прочитать слово из адресного пространства цели (PTRACE_PEEKDATA). */
int pt_read_word(pid_t pid, uintptr_t addr, long *out);

/* TODO: записать слово в адресное пространство цели (PTRACE_POKEDATA). */
int pt_write_word(pid_t pid, uintptr_t addr, long val);

/* TODO: прочитать siginfo (PTRACE_GETSIGINFO) — даёт адрес обращения si_addr. */
int pt_get_siginfo(pid_t pid, uintptr_t *fault_addr);

#endif /* PTRACE_UTIL_H */
