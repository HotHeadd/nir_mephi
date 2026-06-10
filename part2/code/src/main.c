/*
 * Точка входа: разбор аргументов командной строки, запуск агента.
 */
#include "agent.h"
#include "memwatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
    fprintf(stderr,
        "memwatch — опытный образец модуля перехвата записи в память\n\n"
        "Использование:\n"
        "  %s --symbol ИМЯ --len N [--log ФАЙЛ] -- ПРОГРАММА [АРГ...]\n"
        "  %s --addr 0xADDR --len N [--log ФАЙЛ] -- ПРОГРАММА [АРГ...]\n\n"
        "  --symbol ИМЯ   охраняемая область задаётся символом\n"
        "  --addr 0xA     либо абсолютным адресом\n"
        "  --len N        размер области в байтах (по умолчанию 8)\n"
        "  --log ФАЙЛ     журнал событий (по умолчанию stderr)\n",
        prog, prog);
}

int main(int argc, char **argv) {
    agent_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.guard.length = 8;

    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "--symbol") == 0 && i + 1 < argc) {
            cfg.guard.symbol_name = argv[++i];
        } else if (strcmp(argv[i], "--addr") == 0 && i + 1 < argc) {
            cfg.guard.abs_addr = (uintptr_t)strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--len") == 0 && i + 1 < argc) {
            cfg.guard.length = (size_t)strtoull(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            cfg.log_path = argv[++i];
        } else if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        } else {
            fprintf(stderr, "неизвестный аргумент: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    if (i >= argc) {
        fprintf(stderr, "ошибка: не задана целевая программа\n\n");
        usage(argv[0]);
        return 2;
    }
    if (!cfg.guard.symbol_name && !cfg.guard.abs_addr) {
        fprintf(stderr, "ошибка: задайте --symbol или --addr\n\n");
        usage(argv[0]);
        return 2;
    }

    cfg.target_argv = &argv[i];
    return agent_run(&cfg);
}
