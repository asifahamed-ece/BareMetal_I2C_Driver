#include <stdint.h>
#include <stdbool.h>

#include "blackpill_stm32f411.h"

/* PC13 = onboard LED, active low (0 = lit). */
#define LED_PIN      13U
#define LED_PORT     GPIOC

#define HSI_HZ       16000000UL
#define SYSTICK_1MS  (HSI_HZ / 1000UL - 1UL)

static void timebase_init(void) {
    /* No TICKINT: SysTick_Handler is a weak alias to Default_Handler, which
       is `b .`. Enabling the interrupt freezes the chip 1 ms after boot. */
    STK_CSR = 0U;
    STK_RVR = SYSTICK_1MS;
    STK_CVR = 0U;
    STK_CSR = STK_CSR_ENABLE;
}

static void delay_ms(uint32_t ms) {
    while (ms-- > 0U) {
        while ((STK_CSR & STK_CSR_COUNTFLAG) == 0U) {
        }
    }
}

static void led_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    LED_PORT->MODER &= ~(0x3U << (LED_PIN * 2U));
    LED_PORT->MODER |=  (0x1U << (LED_PIN * 2U));

    LED_PORT->BSRR = (1U << LED_PIN);
}

/* BSRR low half sets, high half resets. Atomic, no read-modify-write. */
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
        delay_ms(250);
        led_set(false);
        delay_ms(250);
    }
}
