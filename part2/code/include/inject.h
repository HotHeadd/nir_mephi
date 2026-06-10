/*
 * Инжекция системного вызова mprotect в адресное пространство цели.
 *
 * УЗЛОВОЙ модуль прототипа. Внешний агент не может вызвать mprotect для чужого
 * адресного пространства напрямую: mprotect меняет права ВЫЗЫВАЮЩЕГО процесса.
 * Поэтому агент вынужден заставить саму цель исполнить mprotect:
 *   1) сохранить текущие регистры цели;
 *   2) найти в адресном пространстве цели исполнимую инструкцию syscall;
 *   3) подменить регистры (rax=__NR_mprotect, rdi/rsi/rdx = аргументы) и rip;
 *   4) выполнить один шаг — syscall исполнится в контексте цели;
 *   5) восстановить исходные регистры.
 *
 * Это самое хрупкое место прототипа и основной источник материала для
 * подраздела "Выявленные технические проблемы".
 */
#ifndef INJECT_H
#define INJECT_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

/* TODO: найти адрес инструкции syscall (0x0f05) в исполнимом отображении цели.
 * Возвращает адрес или 0. */
uintptr_t inject_find_syscall_insn(pid_t pid);

/* TODO: выполнить mprotect(addr, len, prot) в контексте цели через инжекцию.
 * syscall_insn — адрес из inject_find_syscall_insn. Возвращает 0 при успехе. */
int inject_mprotect(pid_t pid, uintptr_t syscall_insn,
                    uintptr_t addr, size_t len, int prot);

#endif /* INJECT_H */
