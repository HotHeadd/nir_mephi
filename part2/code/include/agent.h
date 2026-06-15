// Агент-наблюдатель: оркестрация главного цикла мониторинга.
#pragma once

#include "memwatch.h"

// Launch цели, установка охраны, цикл обработки нарушений до завершения.
int agent_run(const agent_config_t *cfg);
