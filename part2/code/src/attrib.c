/*
Атрибуция: привязка адреса инструкции-инициатора к модулю и функции.
Реализует алгоритм методики: /proc/<pid>/maps -> модуль+смещение -> ELF-символ.
*/
#include "attrib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

int attrib_read_maps(pid_t pid, map_entry_t *out, int max) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen maps"); return -1; }

    char line[512];
    int n = 0;
    while (n < max && fgets(line, sizeof(line), f)) {
        map_entry_t e;
        memset(&e, 0, sizeof(e));
        char perms[5] = {0};
        int consumed = 0;
        // start-end perms offset dev inode path
        int got = sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s %" SCNxPTR " %*s %*s %n",
                         &e.start, &e.end, perms, &e.file_offset, &consumed);
        if (got < 4) continue;
        memcpy(e.perms, perms, 4);

        // путь модуля — остаток строки после inode (если есть)
        if (consumed > 0) {
            char *p = line + consumed;
            char *nl = strchr(p, '\n');
            if (nl) *nl = '\0';
            while (*p == ' ') p++;
            strncpy(e.path, p, sizeof(e.path) - 1);
        }
        out[n++] = e;
    }
    fclose(f);
    return n;
}

const map_entry_t *attrib_find_map(const map_entry_t *maps, int n, uintptr_t addr) {
    for (int i = 0; i < n; i++)
        if (addr >= maps[i].start && addr < maps[i].end)
            return &maps[i];
    return NULL;
}

/*
Базовый адрес загрузки модуля: минимальный start среди всех отображений с тем
же путём. Нужен, чтобы перевести виртуальный адрес в смещение, не зависящее от
ASLR, и обратно.
*/
static uintptr_t module_base(const map_entry_t *maps, int n, const char *path) {
    uintptr_t base = (uintptr_t)-1;
    for (int i = 0; i < n; i++)
        if (maps[i].path[0] && strcmp(maps[i].path, path) == 0)
            if (maps[i].start < base) base = maps[i].start;
    return base == (uintptr_t)-1 ? 0 : base;
}

/*
Читаем ELF-файл модуля и ищем функцию по mod_off — смещению внутри модуля
(rip - base). st_value символов приводим к тому же смещению:
  ET_EXEC: st_value — абсолютный VA, смещение = st_value - base;
  ET_DYN : st_value уже является смещением от базы.
*/
static int elf_lookup_symbol_by_offset(const char *module_path, uint64_t mod_off,
                                        int et_dyn, uintptr_t base,
                                        char *out_name, size_t out_sz) {
    int fd = open(module_path, O_RDONLY);
    if (fd < 0) return 0;

    int ok = 0;
    Elf64_Ehdr eh;
    if (read(fd, &eh, sizeof(eh)) != (ssize_t)sizeof(eh)) { close(fd); return 0; }
    if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0) { close(fd); return 0; }

    Elf64_Shdr *sh = calloc(eh.e_shnum, sizeof(Elf64_Shdr));
    if (!sh) { close(fd); return 0; }
    if (pread(fd, sh, eh.e_shnum * sizeof(Elf64_Shdr), eh.e_shoff) !=
        (ssize_t)(eh.e_shnum * sizeof(Elf64_Shdr))) { free(sh); close(fd); return 0; }

    for (int i = 0; i < eh.e_shnum && !ok; i++) {
        if (sh[i].sh_type != SHT_SYMTAB && sh[i].sh_type != SHT_DYNSYM)
            continue;
        Elf64_Shdr *strsh = &sh[sh[i].sh_link];

        char *symtab = malloc(sh[i].sh_size);
        char *strtab = malloc(strsh->sh_size);
        if (!symtab || !strtab) { free(symtab); free(strtab); continue; }

        if (pread(fd, symtab, sh[i].sh_size, sh[i].sh_offset) == (ssize_t)sh[i].sh_size &&
            pread(fd, strtab, strsh->sh_size, strsh->sh_offset) == (ssize_t)strsh->sh_size) {

            int cnt = sh[i].sh_size / sizeof(Elf64_Sym);
            Elf64_Sym *syms = (Elf64_Sym *)symtab;
            for (int s = 0; s < cnt; s++) {
                if (ELF64_ST_TYPE(syms[s].st_info) != STT_FUNC) continue;
                if (syms[s].st_value == 0 || syms[s].st_size == 0) continue;
                // смещение символа от базы модуля
                uint64_t sym_off = et_dyn ? syms[s].st_value
                                          : (syms[s].st_value - base);
                if (mod_off >= sym_off && mod_off < sym_off + syms[s].st_size) {
                    const char *nm = strtab + syms[s].st_name;
                    strncpy(out_name, nm, out_sz - 1);
                    out_name[out_sz - 1] = '\0';
                    ok = 1;
                    break;
                }
            }
        }
        free(symtab);
        free(strtab);
    }

    free(sh);
    close(fd);
    return ok;
}

uintptr_t attrib_resolve_symbol(const map_entry_t *maps, int n, const char *symbol,
                                size_t *out_size) {
    // Ищем символ по имени во всех модулях с непустым путём. Возвращаем
    // фактический виртуальный адрес в адресном пространстве цели.
    if (out_size) *out_size = 0;
    char seen[64][ATTRIB_NAME_MAX];
    int seen_n = 0;

    for (int i = 0; i < n; i++) {
        if (!maps[i].path[0]) continue;
        if (maps[i].path[0] == '[') continue;   // [heap], [stack] и т.п.

        // пропускаем уже обработанные модули
        int dup = 0;
        for (int k = 0; k < seen_n; k++)
            if (strcmp(seen[k], maps[i].path) == 0) { dup = 1; break; }
        if (dup) continue;
        if (seen_n < 64) strncpy(seen[seen_n++], maps[i].path, ATTRIB_NAME_MAX - 1);

        int fd = open(maps[i].path, O_RDONLY);
        if (fd < 0) continue;

        Elf64_Ehdr eh;
        if (read(fd, &eh, sizeof(eh)) != (ssize_t)sizeof(eh) ||
            memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0) { close(fd); continue; }
        int et_dyn = (eh.e_type == ET_DYN);
        uintptr_t base = module_base(maps, n, maps[i].path);

        Elf64_Shdr *sh = calloc(eh.e_shnum, sizeof(Elf64_Shdr));
        if (!sh) { close(fd); continue; }
        if (pread(fd, sh, eh.e_shnum * sizeof(Elf64_Shdr), eh.e_shoff) !=
            (ssize_t)(eh.e_shnum * sizeof(Elf64_Shdr))) { free(sh); close(fd); continue; }

        uintptr_t result = 0;
        for (int s_i = 0; s_i < eh.e_shnum && !result; s_i++) {
            if (sh[s_i].sh_type != SHT_SYMTAB && sh[s_i].sh_type != SHT_DYNSYM)
                continue;
            Elf64_Shdr *strsh = &sh[sh[s_i].sh_link];
            char *symtab = malloc(sh[s_i].sh_size);
            char *strtab = malloc(strsh->sh_size);
            if (!symtab || !strtab) { free(symtab); free(strtab); continue; }

            if (pread(fd, symtab, sh[s_i].sh_size, sh[s_i].sh_offset) == (ssize_t)sh[s_i].sh_size &&
                pread(fd, strtab, strsh->sh_size, strsh->sh_offset) == (ssize_t)strsh->sh_size) {
                int cnt = sh[s_i].sh_size / sizeof(Elf64_Sym);
                Elf64_Sym *syms = (Elf64_Sym *)symtab;
                for (int s = 0; s < cnt; s++) {
                    if (syms[s].st_name == 0) continue;
                    const char *nm = strtab + syms[s].st_name;
                    if (strcmp(nm, symbol) == 0 && syms[s].st_value != 0) {
                        result = et_dyn ? (base + syms[s].st_value)
                                        : (uintptr_t)syms[s].st_value;
                        if (out_size) *out_size = (size_t)syms[s].st_size;
                        break;
                    }
                }
            }
            free(symtab);
            free(strtab);
        }
        free(sh);
        close(fd);
        if (result) return result;
    }
    return 0;
}

int attrib_resolve_addr(const map_entry_t *maps, int n, uintptr_t rip,
                        attrib_result_t *res) {
    memset(res, 0, sizeof(*res));
    const map_entry_t *m = attrib_find_map(maps, n, rip);
    if (!m) return -1;

    // модуль и смещение
    const char *path = m->path[0] ? m->path : "[anonymous]";
    strncpy(res->module, path, sizeof(res->module) - 1);
    uintptr_t base = m->path[0] ? module_base(maps, n, m->path) : m->start;
    res->module_offset = rip - base;

    // попытка разрешить функцию по ELF
    if (m->path[0] && m->path[0] != '[') {
        int fd = open(m->path, O_RDONLY);
        if (fd >= 0) {
            Elf64_Ehdr eh;
            if (read(fd, &eh, sizeof(eh)) == (ssize_t)sizeof(eh) &&
                memcmp(eh.e_ident, ELFMAG, SELFMAG) == 0) {
                int et_dyn = (eh.e_type == ET_DYN);
                // всегда ищем по смещению инструкции внутри модуля
                char fname[ATTRIB_NAME_MAX] = {0};
                if (elf_lookup_symbol_by_offset(m->path, res->module_offset,
                                                et_dyn, base,
                                                fname, sizeof(fname))) {
                    strncpy(res->function, fname, sizeof(res->function) - 1);
                    res->has_function = 1;
                }
            }
            close(fd);
        }
    }
    return 0;
}
