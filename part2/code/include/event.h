/*
 * Запись о перехваченном событии и её вывод в журнал.
 */
#ifndef EVENT_H
#define EVENT_H

#include "attrib.h"
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t        seq;          /* порядковый номер события */
    pid_t           tid;          /* поток-инициатор */
    uintptr_t       fault_addr;   /* адрес обращения к охраняемой области */
    uintptr_t       rip;          /* адрес инструкции-инициатора */
    char            access_type;  /* 'W' для прототипа */
    attrib_result_t attrib;       /* результат атрибуции */
} event_t;

/* TODO: открыть журнал (path или stderr). */
FILE *event_log_open(const char *path);

/* TODO: вывести одну запись о событии в журнал в структурированном виде. */
void event_log_write(FILE *log, const event_t *ev);

/* TODO: закрыть журнал. */
void event_log_close(FILE *log);

#endif /* EVENT_H */
