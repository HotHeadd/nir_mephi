/*
Инжекция системного вызова mprotect в адресное пространство цели.

Внешний агент не может изменить права чужого адресного пространства
напрямую: mprotect действует на вызывающий процесс. Поэтому агент
заставляет цель исполнить mprotect, временно подменив её регистры и
указатель инструкций. Это центральный механизм опытного образца.
*/
#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

// Адрес инструкции syscall (0f 05) в исполнимом отображении цели; 0 если нет.
uintptr_t inject_find_syscall_insn(pid_t pid);

// Выполнить mprotect(addr, len, prot) в контексте цели через инжекцию.
// syscall_insn — адрес из inject_find_syscall_insn. 0 при успехе.
int inject_mprotect(pid_t pid, uintptr_t syscall_insn,
                    uintptr_t addr, size_t len, int prot);
