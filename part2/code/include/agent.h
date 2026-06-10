/*
 * Агент-наблюдатель: оркестрация главного цикла мониторинга.
 * Связывает перехват (ptrace_util), установку охраны (guard/inject),
 * атрибуцию (attrib) и вывод (event).
 */
#ifndef AGENT_H
#define AGENT_H

#include "memwatch.h"

/* Запуск агента по конфигурации: launch цели, установка охраны,
 * цикл обработки нарушений до завершения цели. Возвращает код выхода. */
int agent_run(const agent_config_t *cfg);

#endif /* AGENT_H */
