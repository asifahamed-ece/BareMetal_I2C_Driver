#include "blackpill_stm32f411.h"
#include "timebase.h"

void timebase_init()
{
    CoreDebug_DEMCR |= TRCENA ; // TRACE ENABLE Bit 24.
    DWT_CTRL |= CYCCNTENA;  // Cyclic Counter Enable bit 0.
}

uint32_t timebase_us(){
    return DWT_CYCCNT /100;
}

void delay_us(uint32_t us)
{
    uint32_t start_time = timebase_us();
    while( (timebase_us() - start_time) < us);
}