/*
 * Установка и снятие охраны над объявленной областью.
 * Разрешение символического имени области в фактический виртуальный адрес.
 */
#ifndef GUARD_H
#define GUARD_H

#include "memwatch.h"
#include <sys/types.h>

/* TODO: разрешить guard_spec (символ или абс. адрес) в фактический адрес
 * в адресном пространстве цели pid. Заполняет region. Возвращает 0 при успехе.
 * Использует attrib_* (maps + ELF) для разрешения символа. */
int guard_resolve(pid_t pid, const guard_spec_t *spec, guard_region_t *region);

/* TODO: установить охрану — снять право записи (перевести в PROT_READ).
 * Через inject_mprotect. */
int guard_protect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region);

/* TODO: временно вернуть право записи (PROT_READ|PROT_WRITE) перед single-step. */
int guard_unprotect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region);

/* TODO: проверить, попадает ли адрес обращения в охраняемую область. */
int guard_contains(const guard_region_t *region, uintptr_t fault_addr);

#endif /* GUARD_H */
