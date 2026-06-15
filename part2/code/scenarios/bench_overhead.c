/*
 * Сценарий оценки накладных расходов (overhead).
 *
 * В отличие от пяти функциональных сценариев, где происходит ровно одно
 * обращение к охраняемой области, эта цель совершает N записей в критическую
 * переменную в горячем цикле. Каждая запись (когда область под охраной)
 * порождает полный цикл обработки нарушения в агенте, поэтому суммарная
 * задержка, делённая на N, даёт среднюю стоимость одного перехвата.
 *
 * Цель сама замеряет своё время через CLOCK_MONOTONIC и печатает его,
 * чтобы замер не зависел от стоимости запуска агента. Внешний `time`
 * служит независимой проверкой.
 *
 * Запускается в двух режимах одной и той же сборкой:
 *   - БЕЗ агента: цикл выполняется штатно, замер = базовое время цели;
 *   - ПОД агентом с охраной critical_flag: каждая запись перехватывается.
 *
 * Число итераций задаётся аргументом (по умолчанию 100000).
 * Цель собирается no-pie для стабильности адреса (как s4), но агент может
 * охранять и по --symbol critical_flag.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

volatile uint64_t critical_flag = 0;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
    long n = (argc > 1) ? strtol(argv[1], NULL, 10) : 100000;
    if (n <= 0) n = 100000;

    printf("[bench] адрес critical_flag = %p\n", (void*)&critical_flag);
    printf("[bench] итераций N = %ld\n", n);

    double t0 = now_sec();
    /* горячий цикл записи в охраняемую область */
    for (long i = 0; i < n; i++) {
        critical_flag = (uint64_t)i;   /* каждая запись — потенциальное срабатывание */
    }
    double t1 = now_sec();

    double total = t1 - t0;
    printf("[bench] суммарное время цикла : %.6f c\n", total);
    printf("[bench] на одну итерацию      : %.3f мкс\n", total / (double)n * 1e6);
    printf("[bench] финальное значение    : 0x%lx\n", (unsigned long)critical_flag);
    return 0;
}