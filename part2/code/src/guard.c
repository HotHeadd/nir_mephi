/*
 * Установка/снятие охраны над объявленной областью и разрешение её адреса.
 */
#include "guard.h"
#include "inject.h"
#include "attrib.h"

#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_MAPS 1024

static size_t page_size_cached(void) {
    long ps = sysconf(_SC_PAGESIZE);
    return ps > 0 ? (size_t)ps : 4096;
}

int guard_resolve(pid_t pid, const guard_spec_t *spec, guard_region_t *region) {
    memset(region, 0, sizeof(*region));

    uintptr_t addr = 0;
    if (spec->symbol_name) {
        map_entry_t maps[MAX_MAPS];
        int n = attrib_read_maps(pid, maps, MAX_MAPS);
        if (n <= 0) return -1;
        addr = attrib_resolve_symbol(maps, n, spec->symbol_name);
        if (!addr) {
            fprintf(stderr, "guard_resolve: символ '%s' не найден\n", spec->symbol_name);
            return -1;
        }
    } else if (spec->abs_addr) {
        addr = spec->abs_addr;
    } else {
        fprintf(stderr, "guard_resolve: не задан ни символ, ни адрес\n");
        return -1;
    }

    size_t ps = page_size_cached();
    region->addr = addr;
    region->length = spec->length ? spec->length : 8;
    region->page_addr = addr & ~(ps - 1);
    /* сколько байт страниц покрывает область [addr, addr+length) */
    uintptr_t last = addr + region->length - 1;
    uintptr_t last_page = last & ~(ps - 1);
    region->page_span = (last_page - region->page_addr) + ps;
    return 0;
}

int guard_protect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region) {
    /* снять право записи: страницы области -> только чтение */
    return inject_mprotect(pid, syscall_insn,
                           region->page_addr, region->page_span, PROT_READ);
}

int guard_unprotect(pid_t pid, uintptr_t syscall_insn, const guard_region_t *region) {
    /* временно вернуть запись для одиночного шага */
    return inject_mprotect(pid, syscall_insn,
                           region->page_addr, region->page_span,
                           PROT_READ | PROT_WRITE);
}

int guard_contains(const guard_region_t *region, uintptr_t fault_addr) {
    if (!region) return 0;
    return fault_addr >= region->addr &&
           fault_addr <  region->addr + region->length;
}
