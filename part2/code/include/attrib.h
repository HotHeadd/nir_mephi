/*
 * Атрибуция: привязка адреса инструкции-инициатора к модулю и функции.
 * Реализует алгоритм методики (глава 1): maps -> модуль+смещение -> ELF-символ.
 */
#ifndef ATTRIB_H
#define ATTRIB_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#define ATTRIB_NAME_MAX 256

/* Одна строка /proc/<pid>/maps. */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t file_offset;
    char      perms[5];                  /* "rwxp" */
    char      path[ATTRIB_NAME_MAX];     /* путь к модулю или "" */
} map_entry_t;

/* Результат атрибуции одного адреса. */
typedef struct {
    char      module[ATTRIB_NAME_MAX];   /* имя модуля */
    uintptr_t module_offset;             /* смещение внутри модуля */
    char      function[ATTRIB_NAME_MAX]; /* имя функции или "" если не разрешено */
    int       has_function;              /* 1 если функция разрешена по ELF */
} attrib_result_t;

/* TODO: прочитать и распарсить /proc/<pid>/maps в массив. Возвращает кол-во. */
int attrib_read_maps(pid_t pid, map_entry_t *out, int max);

/* TODO: найти запись maps, в чьи границы попадает addr. NULL если нет. */
const map_entry_t *attrib_find_map(const map_entry_t *maps, int n, uintptr_t addr);

/* TODO: по символу symbol найти его адрес в адресном пространстве цели,
 * сопоставив ELF-символ модуля с базой отображения из maps. 0 если не найден. */
uintptr_t attrib_resolve_symbol(const map_entry_t *maps, int n, const char *symbol);

/* TODO: полная атрибуция адреса rip: модуль, смещение, имя функции (по ELF). */
int attrib_resolve_addr(const map_entry_t *maps, int n, uintptr_t rip,
                        attrib_result_t *res);

#endif /* ATTRIB_H */
