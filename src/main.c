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
#include "i2c.h"
#include "ssd1306.h"

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

static void print_hex(uint8_t val){
    char hex[] = "0123456789ABCDEF";
    uart_write_char(hex[(val >> 4) & 0x0F]);
    uart_write_char(hex[val & 0x0F]);
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

    i2c_init();
    uart_print("Scanning I2C Bus...\r\n");

    // uart_print("Comms Test !\r\n");
    // uart_print("I am BlackPill, Is there anyone On-Range!!!\r\n");

    for(uint8_t addr = 0x08; addr < 0x78; addr++){
        if(i2c_probe(addr)){
            uart_print("Found: 0x");
            print_hex(addr);
            uart_print("\r\n");
        }
    }
    uart_print("Scanning done...!\r\n");

    uart_print("Initializing OLED...\r\n");
    delay_us(100000);       // Wait 100ms for the OLED power to stabilize
    SSD1306_Init();         // Send the init sequence (from your ssd1306.c)
    uart_print("OLED Initialized\r\n");

    while (1) {
        // --- STATE 1: EVERYTHING ON ---
        led_set(true);                      // LED ON
        OLED_Fill(0xFF);                    // Fill screen with White
        uart_print("LED ON / Screen WHITE\r\n");
        delay_us(2000000);                  // Wait 2 seconds

        // --- STATE 2: EVERYTHING OFF ---
        led_set(false);                     // LED OFF
        OLED_Fill(0x00);                    // Fill screen with Black
        uart_print("LED OFF / Screen BLACK\r\n");
        delay_us(2000000);                  // Wait 2 seconds
    }
}