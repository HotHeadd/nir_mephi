/*
Модельная защищаемая программа (испытательный стенд).
Содержит критическую переменную critical_flag, которая после инициализации
не должна изменяться. Однопоточная. Собирается без strip — символы
доступны атрибуции.
*/
#include <stdio.h>
#include <stdint.h>

volatile uint64_t critical_flag = 0;

// Легитимная инициализация.
void init_flag(uint64_t v) {
    critical_flag = v;
}

// Недопустимое изменение критической переменной после фиксации области.
void illegitimate_write(uint64_t v) {
    critical_flag = v;
}

int main(void) {
    init_flag(0x1122334455667788ULL);
    printf("[target] critical_flag initialized = 0x%lx\n",
           (unsigned long)critical_flag);

    illegitimate_write(0xdeadbeefULL);   // ожидается перехват
    printf("[target] after illegitimate_write = 0x%lx\n",
           (unsigned long)critical_flag);

    return 0;
}
