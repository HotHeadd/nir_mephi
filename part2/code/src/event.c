/*
 * Запись о перехваченном событии и её вывод в журнал.
 */
#include "event.h"
#include <stdio.h>
#include <inttypes.h>

FILE *event_log_open(const char *path) {
    if (!path) return stderr;
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen log");
        return stderr;
    }
    return f;
}

void event_log_write(FILE *log, const event_t *ev) {
    if (!log || !ev) return;
    fprintf(log,
        "событие #%" PRIu64 " | TID=%d | доступ=%c\n"
        "  адрес обращения : 0x%" PRIxPTR "\n"
        "  инструкция (RIP): 0x%" PRIxPTR "\n"
        "  модуль          : %s + 0x%" PRIxPTR "\n",
        ev->seq, (int)ev->tid, ev->access_type,
        ev->fault_addr, ev->rip,
        ev->attrib.module, ev->attrib.module_offset);
    if (ev->attrib.has_function)
        fprintf(log, "  функция         : %s\n", ev->attrib.function);
    else
        fprintf(log, "  функция         : <не разрешена (нет символов)>\n");
    fprintf(log, "\n");
    fflush(log);
}

void event_log_close(FILE *log) {
    if (log && log != stderr && log != stdout) fclose(log);
}
