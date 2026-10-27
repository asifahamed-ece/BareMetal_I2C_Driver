/**
 * @file    timebase.c
 * @brief   Microsecond timebase from the Cortex-M4 Data Watchpoint and Trace
 *          cycle counter.
 *
 * Reference: ARMv7-M Architecture Reference Manual, DWT chapter and DEMCR in
 *            the CoreDebug chapter. DWT is a core peripheral, so it is not
 *            documented in RM0383.
 *
 * Why DWT and not SysTick: SysTick raises an interrupt, and an unhandled
 * vector parks the CPU in Default_Handler forever. DWT_CYCCNT just counts
 * silently in hardware, so a timebase built on it cannot itself hang the chip.
 */

#include "blackpill_stm32f411.h"
#include "timebase.h"

/**
 * @brief   Enable the trace/debug blocks and start the cycle counter.
 *
 * @note    Two separate enables are required and both are easy to forget:
 *          CoreDebug_DEMCR_TRCENA gates the whole DWT block, DWT_CTRL_CYCCNTENA
 *          gates the counter inside it. Set only CYCCNTENA and it never increments.
 */
void timebase_init(void)
{
    CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA;      /* DEMCR bit 24: power on the DWT block. */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;                 /* DWT_CTRL bit 0: start counting HCLK. */
}

/**
 * @brief   Current time in microseconds.
 * @return  DWT_CYCCNT / 100, i.e. cycles divided into 1 MHz ticks.
 *
 * @note    The 100 is not magic: HCLK is 100 MHz, so 100 cycles = 1 us.
 *          The compiler turns `x / 100` into a multiply by 0x51EB851F and a
 *          shift, which is why this is cheap enough to call in a tight loop.
 */
uint32_t timebase_us(void)
{
    return DWT_CYCCNT / 100U;
}

/**
 * @brief   Busy-wait for @p us microseconds.
 * @param   us Delay length in microseconds.
 *
 * @note    The subtraction is the whole trick. CYCCNT is a 32-bit counter
 *          that wraps every 2^32 cycles (~42.9 s at 100 MHz), and unsigned
 *          arithmetic makes `now - start` give the correct elapsed count
 *          across that wrap. Comparing `now < start` instead would break once
 *          per 42.9 s, roughly never while testing and always in the field.
 */
void delay_us(uint32_t us)
{
    uint32_t start_time = timebase_us();

    while ((timebase_us() - start_time) < us) {
        /* spin */
    }
}
