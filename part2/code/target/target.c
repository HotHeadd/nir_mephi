/*
 * Модельная защищаемая программа (испытательный стенд).
 * Содержит критическую переменную critical_flag, которая после инициализации
 * не должна изменяться. Разные сценарии валидации обращаются к ней по-разному.
 *
 * Однопоточная (допущение прототипа). Символ critical_flag должен быть видим
 * в таблице символов ELF (собирать без -s, не strip).
 */
#include <stdio.h>
#include <stdint.h>

/* Критическая переменная — кандидат на охрану. Глобальная, чтобы иметь
 * стабильный символ в .symtab/.dynsym. */
volatile uint64_t critical_flag = 0;

/* Легитимная инициализация: запись ДО установки охраны. */
void init_flag(uint64_t v) {
    critical_flag = v;
}

/* Нарушитель: запись ПОСЛЕ фиксации области (моделирует недопустимое
 * изменение критической переменной). */
void illegitimate_write(uint64_t v) {
    critical_flag = v;
}

int main(void) {
    init_flag(0x1122334455667788ULL);
    printf("[target] critical_flag initialized = 0x%lx\n",
           (unsigned long)critical_flag);

    /* TODO: точка синхронизации с агентом — здесь агент успевает установить
     * охрану. В прототипе порядок обеспечивается остановками ptrace. */

    illegitimate_write(0xdeadbeefULL);   /* ожидается перехват */
    printf("[target] after illegitimate_write = 0x%lx\n",
           (unsigned long)critical_flag);

    return 0;
}
