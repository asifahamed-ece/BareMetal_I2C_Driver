/**
 * @file    main.c
 * @brief   Phase 0.1 smoke test: blink the onboard LED on PC13.
 */

#include <stdint.h>
#include <stdbool.h>

#include "blackpill_stm32f411.h"

#define LED_PIN      13U      ///< Onboard LED, active low (0 = lit)
#define LED_PORT     GPIOC

#define HSI_HZ       16000000UL
#define SYSTICK_1MS  ((HSI_HZ / 1000UL) - 1UL)

/**
 * @brief Start SysTick as a 1 ms down-counter on the processor clock.
 * @warning TICKINT stays clear. SysTick_Handler is a weak alias to
 *          Default_Handler (`b .`), so enabling the interrupt freezes the
 *          chip one millisecond after boot.
 */
static void timebase_init(void) {
    STK_CSR = 0U;
    STK_RVR = SYSTICK_1MS;
    STK_CVR = 0U;
    STK_CSR = STK_CSR_ENABLE | STK_CSR_CLKSOURCE;
}

/** @brief Blocking delay, polling COUNTFLAG. @param ms Milliseconds. */
static void delay_ms(uint32_t ms) {
    while (ms > 0U) {
        while ((STK_CSR & STK_CSR_COUNTFLAG) == 0U) {
        }
        ms--;
    }
}

/** @brief Configure PC13 as a push-pull output, starting dark. */
static void led_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    LED_PORT->MODER &= ~(0x3U << (LED_PIN * 2U));
    LED_PORT->MODER |=  (0x1U << (LED_PIN * 2U));

    LED_PORT->BSRR = (1U << LED_PIN);
}

/**
 * @brief Set the LED state.
 * @param on true = lit, false = dark.
 * @note BSRR is atomic: low half sets, high half resets. No read-modify-write.
 */
static void led_set(bool on) {
    if (on) {
        LED_PORT->BSRR = (1U << (LED_PIN + 16U));
    } else {
        LED_PORT->BSRR = (1U << LED_PIN);
    }
}

int main(void) {
    led_init();
    timebase_init();

    while (1) {
        led_set(true);
        delay_ms(1000);
        led_set(false);
        delay_ms(100);
    }
}
