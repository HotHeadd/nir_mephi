/*
Сценарий 2: запись из постороннего модуля.
critical_flag объявлен здесь, но пишет в него функция из чужой .so.
Ожидание: событие зарегистрировано, атрибуция указывает на модуль
libs2attacker.so (а не на основную программу).
*/
#include <stdint.h>
#include <stdio.h>

volatile uint64_t critical_flag = 0;

// объявлена в библиотеке-нарушителе
extern void attacker_write(volatile uint64_t *target);

int main(void) {
    printf("[s2] critical_flag = 0x%lx, зовём чужой модуль\n",
           (unsigned long)critical_flag);
    attacker_write(&critical_flag);   // запись произойдёт ИЗ .so
    printf("[s2] после чужого модуля = 0x%lx\n", (unsigned long)critical_flag);
    return 0;
}
