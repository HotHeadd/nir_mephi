// Запись о перехваченном событии и её вывод в журнал.
#pragma once

#include "attrib.h"
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t        seq;
    pid_t           tid;
    uintptr_t       fault_addr;
    uintptr_t       rip;
    char            access_type;   // 'W' для опытного образца
    attrib_result_t attrib;
} event_t;

FILE *event_log_open(const char *path);
void  event_log_write(FILE *log, const event_t *ev);
void  event_log_close(FILE *log);
