/**
 * @file    clock.h
 * @brief   System clock configuration for the STM32F411.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

/**
 * @brief Bring SYSCLK to 100 MHz from HSI through the PLL.
 *
 * Resulting tree: HSI 16 MHz -> PLL -> SYSCLK 100 MHz, AHB 100 MHz,
 * APB1 50 MHz, APB2 100 MHz. Sets VOS scale 1 and 3 flash wait states.
 *
 * @warning Blocking, with no timeout. Spins on PLLRDY and on the SWS
 *          read-back, so it hangs if the PLL never locks and must not be
 *          called from an interrupt context.
 *
 * @note Call this before anything that derives timing from SYSCLK. SysTick
 *       reload values and I2C CCR/TRISE computed for 16 MHz are invalid
 *       afterwards.
 */
void clock_init_100Mhz();

/**
 * @brief PCLK1 frequency in Hz, for peripheral timing math.
 * @return 50,000,000 after clock_init_100Mhz().
 * @note Hardcoded, not read from RCC_CFGR. Must be updated by hand if the
 *       APB1 prescaler ever changes.
 */
uint32_t clock_get_pclk1_hz(void);

#endif /* CLOCK_H */
