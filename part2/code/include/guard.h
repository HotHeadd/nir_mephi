/*
Установка и снятие охраны над объявленной областью.
Разрешение символического имени области в фактический виртуальный адрес.
*/
#pragma once

#include "memwatch.h"
#include <sys/types.h>

// Разрешить spec (символ или абсолютный адрес) в фактический адрес в
// адресном пространстве цели; заполнить region. 0 при успехе.
int guard_resolve(pid_t pid, const guard_spec_t *spec, guard_region_t *region);

// Снять право записи с охраняемой области (перевести в PROT_READ).
int guard_protect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region);

// Временно вернуть право записи перед одиночным шагом.
int guard_unprotect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region);

// Попадает ли адрес обращения в охраняемую область.
int guard_contains(const guard_region_t *region, uintptr_t fault_addr);
