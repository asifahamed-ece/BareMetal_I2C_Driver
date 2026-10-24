/**
 * @file    main.c
 * @brief   Phase 0.1 smoke test: blink the onboard LED on PC13.
 */

#include <stdint.h>
#include <stdbool.h>

#include "blackpill_stm32f411.h"
#include "clock.h"
#include "timebase.h"
#include "uart.h"

#define LED_PIN      13U      ///< Onboard LED, active low (0 = lit)
#define LED_PORT     GPIOC


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

/**
 * @brief Blink the onboard LED: 100 ms lit, 1 s dark.
 *
 * clock_init_100Mhz() runs first, so SysTick and every delay below are
 * derived from 100 MHz rather than the 16 MHz reset default.
 */
int main(void) {

    clock_init_100Mhz();    
    timebase_init();
    led_init();
    uart2_init();

    // uart_print("Comms Test !\r\n");
    // uart_print("I am BlackPill, Is there anyone On-Range!!!\r\n");
    
    while (1) {
        uart_write_char('U');  // Send exactly one byte
        led_set(true);
        delay_us(1000000);
        led_set(false);
        delay_us(1000000);
    }
}
