/* Сценарий 4: переполнение соседнего буфера затирает critical_flag. */
#include <stdint.h>
#include <string.h>
volatile uint64_t critical_flag = 0;
char neighbor[8];
int main(void) {
    /* TODO: запись за границу neighbor, дотягивающаяся до critical_flag.
     * Раскладку проверить через nm/readelf — соседство не гарантировано
     * компоновщиком, это само по себе материал для "технических проблем". */
    memset(neighbor, 0x41, 16);   /* намеренный выход за границу (модельно) */
    return 0;
}
