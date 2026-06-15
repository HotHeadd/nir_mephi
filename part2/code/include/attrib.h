/*
Атрибуция: привязка адреса инструкции-инициатора к модулю и функции.
Алгоритм: maps -> модуль + смещение -> ELF-символ.
*/
#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#define ATTRIB_NAME_MAX 256

// Одна строка /proc/<pid>/maps.
typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t file_offset;
    char      perms[5];
    char      path[ATTRIB_NAME_MAX];
} map_entry_t;

// Результат атрибуции одного адреса.
typedef struct {
    char      module[ATTRIB_NAME_MAX];
    uintptr_t module_offset;
    char      function[ATTRIB_NAME_MAX];
    int       has_function;             // 1 если функция разрешена по ELF
} attrib_result_t;

// Прочитать /proc/<pid>/maps в массив; возвращает число записей.
int attrib_read_maps(pid_t pid, map_entry_t *out, int max);

// Запись maps, в чьи границы попадает addr; NULL если нет.
const map_entry_t *attrib_find_map(const map_entry_t *maps, int n, uintptr_t addr);

// Адрес символа в адресном пространстве цели по ELF модуля; 0 если не найден.
// Если out_size != NULL, в него записывается размер символа (st_size), 0 если неизвестен.
uintptr_t attrib_resolve_symbol(const map_entry_t *maps, int n, const char *symbol,
                                size_t *out_size);

// Полная атрибуция адреса rip: модуль, смещение, имя функции (по ELF).
int attrib_resolve_addr(const map_entry_t *maps, int n, uintptr_t rip,
                        attrib_result_t *res);
