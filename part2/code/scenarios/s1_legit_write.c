// Сценарий 1: легитимная запись до фиксации. Ложных срабатываний быть не должно.
#include <stdint.h>
volatile uint64_t critical_flag = 0;
int main(void) {
    critical_flag = 1;   // легитимная запись; с --allow main не регистрируется как нарушение
    return 0;
}
