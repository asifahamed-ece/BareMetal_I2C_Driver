/**
 * @file    main.c
 * @brief   I2C bus scanner and SSD1306 demo for the baremetal I2C1 master.
 *
 * Bring-up order matters: clock first, then the timebase (all delays and all
 * I2C timeouts derive from it), then the GPIO the demo needs, then UART for
 * reporting, and I2C last so it is configured against a settled clock.
 *
 * Output goes to PA2 at 115200 8N1 and is mirrored by the ESP32 bridge in
 * docs/UART_RX_ESP32. A device on the bus is reported by its 7-bit address.
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
#define LED_PORT     GPIOC    ///< PC13 sits on GPIOC


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
 * @brief Print one byte as two uppercase hex digits over UART.
 * @param val Byte to print, 0x00..0xFF.
 * @note Always two digits, so addresses stay column-aligned in the scan list.
 */
static void print_hex(uint8_t val){
    char hex[] = "0123456789ABCDEF";
    uart_write_char(hex[(val >> 4) & 0x0F]);
    uart_write_char(hex[val & 0x0F]);
}

/**
 * @brief Probe every legal 7-bit address and report the ones that answer.
 * @return Number of devices that acknowledged.
 * @note Runs the full 0x08..0x77 sweep every time; there is no shortcut for
 *       "is anything there". Each probe is a complete START/address/STOP.
 */
static uint8_t scan_bus(void)
{
    uint8_t found = 0;

    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_probe(addr)) {
            uart_print("Found device at: 0x");
            print_hex(addr);
            uart_print("\r\n");
            found++;
        }
    }

    return found;
}


/**
 * @brief Scan the I2C bus, then alternate the OLED fill and the onboard LED.
 *
 * The scan walks every legal 7-bit address. If nothing answers, recovery is
 * attempted once, because an empty bus after a successful earlier run usually
 * means a slave is holding SDA rather than that the wiring fell off.
 *
 * clock_init_100Mhz() runs first, so every delay below and every I2C timeout is
 * measured against 100 MHz rather than the 16 MHz reset default.
 */
int main(void) {
    clock_init_100Mhz();    
    timebase_init();
    led_init();
    uart2_init();
    i2c_init();

    uart_print("\r\n--- BareMetal I2C Driver Initialized ---\r\n");

    // Scan the bus
    uart_print("Scanning I2C Bus...\r\n");
    uint8_t devices_found = scan_bus();

    if (devices_found == 0) {
        uart_print("No devices found. Attempting bus recovery...\r\n");

        if (i2c_bus_recover()) {
            uart_print("SDA released. Rescanning...\r\n");
            devices_found = scan_bus();
        } else {
            uart_print("Recovery failed: SDA is still held low.\r\n");
        }
    }

    if (devices_found == 0) {
        uart_print("Warning: nothing answered on the bus.\r\n");
    }

    // Init OLED
    uart_print("Initializing OLED...\r\n");
    delay_us(100000);       
    SSD1306_Init();         

    // Demo Loop
    while (1) {
        led_set(true);                      
        OLED_Fill(0xFF);                    // Fill screen White
        uart_print("LED ON / Screen WHITE\r\n");
        delay_us(2000000);                  

        led_set(false);                     
        OLED_Fill(0x00);                    // Fill screen Black
        uart_print("LED OFF / Screen BLACK\r\n");
        delay_us(2000000);                  
    }
}