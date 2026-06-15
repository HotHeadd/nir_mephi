// Обёртки над ptrace/waitpid: изолируют примитивы перехвата от логики агента.
#include "ptrace_util.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

pid_t pt_launch(char **argv) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        // Потомок объявляет себя трассируемым: первый execve остановит процесс
        // с SIGTRAP, дав агенту точку для установки охраны до первого
        // обращения к защищаемой области.
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("PTRACE_TRACEME");
            _exit(127);
        }
        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }
    return pid;
}

int pt_wait(pid_t pid, stop_info_t *info) {
    int status;
    if (info) memset(info, 0, sizeof(*info));
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        if (info) info->reason = STOP_UNKNOWN;
        return -1;
    }
    if (!info) return 0;

    if (WIFEXITED(status)) {
        info->reason = STOP_EXITED;
        info->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        info->reason = STOP_SIGNALED;
        info->signal = WTERMSIG(status);
    } else if (WIFSTOPPED(status)) {
        info->reason = STOP_SIGNAL;
        info->signal = WSTOPSIG(status);
    } else {
        info->reason = STOP_UNKNOWN;
    }
    return 0;
}

int pt_continue(pid_t pid, int sig) {
    if (ptrace(PTRACE_CONT, pid, NULL, (void *)(long)sig) < 0) {
        perror("PTRACE_CONT");
        return -1;
    }
    return 0;
}

int pt_singlestep(pid_t pid, int sig) {
    if (ptrace(PTRACE_SINGLESTEP, pid, NULL, (void *)(long)sig) < 0) {
        perror("PTRACE_SINGLESTEP");
        return -1;
    }
    return 0;
}

int pt_get_regs(pid_t pid, struct user_regs_struct *regs) {
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
        perror("PTRACE_GETREGS");
        return -1;
    }
    return 0;
}

int pt_set_regs(pid_t pid, const struct user_regs_struct *regs) {
    if (ptrace(PTRACE_SETREGS, pid, NULL, (void *)regs) < 0) {
        perror("PTRACE_SETREGS");
        return -1;
    }
    return 0;
}

int pt_get_siginfo(pid_t pid, uintptr_t *fault_addr) {
    siginfo_t si;
    memset(&si, 0, sizeof(si));
    if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &si) < 0) {
        perror("PTRACE_GETSIGINFO");
        return -1;
    }
    // при SIGSEGV si_addr — адрес, обращение к которому вызвало нарушение
    if (fault_addr) *fault_addr = (uintptr_t)si.si_addr;
    return 0;
}
