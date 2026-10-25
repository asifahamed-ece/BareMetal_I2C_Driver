/**
 * @file    timebase.h
 * @brief   Microsecond timebase built on the Cortex-M4 cycle counter (DWT).
 *
 * There is no free-running timer here. DWT_CYCCNT increments once per HCLK
 * cycle, so at 100 MHz one microsecond is 100 cycles and division gives the
 * time. That makes the timebase exactly as accurate as the system clock —
 * which on this board is HSI, an RC oscillator, and it shows.
 */

#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

/**
 * @brief   Enable DWT and start the cycle counter.
 * @note    Call once, after clock_init_100Mhz(). DEMCR.TRCENA must be set
 *          before the DWT block will count at all, even on Cortex-M4.
 */
void timebase_init(void);

/**
 * @brief   Current time in microseconds, wrapping every ~42.9 s.
 * @return  Monotonic microsecond counter.
 *
 * @warning The divisor is hardcoded to 100, i.e. it assumes HCLK = 100 MHz.
 *          Change the clock tree and every delay silently changes with it.
 * @note    Compare with unsigned subtraction, never with `<` on raw values:
 *          `timebase_us() - start` stays correct across the wrap.
 */
uint32_t timebase_us(void);

/**
 * @brief   Busy-wait for @p us microseconds.
 * @param   us Delay length; must be well below the ~42.9 s wrap.
 * @note    Granularity is 1 us, and the call overhead is a few microseconds.
 *          This burns power at full clock; it is a bring-up tool, not a
 *          power-friendly sleep.
 */
void delay_us(uint32_t us);

#endif /* TIMEBASE_H */
