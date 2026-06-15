/*
Сценарий 3: запись из посторонней функции того же модуля.
Нарушитель — writer_foreign в том же модуле. Ожидание: атрибуция различает
инициатора на уровне ФУНКЦИИ (writer_foreign), а не только модуля.
*/
#include <stdint.h>
#include <stdio.h>

volatile uint64_t critical_flag = 0;

// посторонняя функция-нарушитель того же модуля
void writer_foreign(uint64_t v) {
    critical_flag = v;
}

int main(void) {
    printf("[s3] critical_flag = 0x%lx\n", (unsigned long)critical_flag);
    writer_foreign(0xF00D);   // нарушение из writer_foreign
    printf("[s3] после writer_foreign = 0x%lx\n", (unsigned long)critical_flag);
    return 0;
}
