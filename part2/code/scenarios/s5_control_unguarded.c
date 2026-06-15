/*
Сценарий 5: избирательность охраны.
Есть critical_flag (его агент охраняет) и other_var (НЕ охраняется).
Программа пишет ТОЛЬКО в other_var. Ожидание: агент не регистрирует
НИ ОДНОГО события, потому что охраняемой области никто не касался.
*/
#include <stdint.h>
#include <stdio.h>

volatile uint64_t critical_flag = 0;   // агент будет охранять это
volatile uint64_t other_var     = 0;   // а пишем сюда — это не охраняется

int main(void) {
    other_var = 0xAABBCCDD;             // запись мимо охраны — событий быть не должно
    printf("[s5] other_var = 0x%lx, critical_flag не тронут\n",
           (unsigned long)other_var);
    return 0;
}
