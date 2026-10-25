/**
 * @file    uart.c
 * @brief   Baremetal USART2, 115200 8N1, polling transmit.
 *
 * Reference: RM0383 §19 (p505); baud rate generation §19.3.4.
 *
 * The three registers that matter:
 *   CR1  - UE/TE/RE: enable the peripheral, the transmitter, the receiver
 *   BRR  - the divisor that turns PCLK1 into the bit clock
 *   SR   - TXE: "DR is free, give me another byte"
 * Get BRR wrong and everything else still looks correct while the receiver
 * sees noise; that is why the baud maths below is worth reading carefully.
 */

#include "blackpill_stm32f411.h"
#include "uart.h"

#define UART2_TX      2U            ///< PA2, USART2_TX, alternate function 7
#define UART2_RX      3U            ///< PA3, USART2_RX, alternate function 7
#define PCLK1_FREQ    50000000U     ///< APB1 = 50 MHz, set by clock_init_100Mhz()
#define TARGET_BAUD   115200U       ///< Must match the receiver exactly

/**
 * @brief   Route PA2/PA3 to USART2 and start it at @ref TARGET_BAUD.
 *
 * @note    Must run after clock_init_100Mhz(): @ref PCLK1_FREQ is a constant,
 *          not a measurement, so if APB1 ever changes this silently becomes
 *          wrong and the baud drifts with it.
 */
void uart2_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_USART2EN;

    /* MODER: 0b10 = alternate function. The peripheral drives the pin, not
     * the GPIO output register. */
    GPIOA_MODER &= ~((0x03U << GPIO_PIN_MODE_SHIFT(UART2_TX)) | (0x03U << GPIO_PIN_MODE_SHIFT(UART2_RX)));
    GPIOA_MODER |=  ((0x02U << GPIO_PIN_MODE_SHIFT(UART2_TX)) | (0x02U << GPIO_PIN_MODE_SHIFT(UART2_RX)));

    /* AFRL: PA2 occupies bits 11:8, PA3 bits 15:12, four bits each. AF7 is
     * USART2 on this part (datasheet Table 9). Hardcoded to these two pins. */
    GPIOA_AFRL &= ~((0xFU << 8U) | (0xFU << 12U));
    GPIOA_AFRL |=  ((0x7U << 8U) | (0x7U << 12U));

    /*
     * Baud rate. With oversampling by 16 (OVER8 = 0):
     *     baud      = PCLK1 / (16 x USARTDIV)
     *     USARTDIV  = PCLK1 / (16 x baud)
     * BRR stores USARTDIV in 12.4 fixed point, i.e. its raw value is
     * USARTDIV x 16 - so the 16 above cancels and BRR is simply PCLK1 / baud.
     *
     *     50 000 000 / 115 200 = 434.03  ->  434  (= 0x1B2)
     *     mantissa 27, fraction 2  ->  USARTDIV = 27.125
     *     actual baud = 50e6 / (16 x 27.125) = 115 207   (+0.006 %)
     *
     * Adding baud/2 first rounds to nearest instead of truncating, which
     * halves the worst-case error. Truncation alone would give 434 anyway
     * here, but the habit matters at other rates.
     */
    uint32_t brr_value = (PCLK1_FREQ + (TARGET_BAUD / 2U)) / TARGET_BAUD;
    USART2_BRR = brr_value;

    /* UE last: the peripheral latches its configuration on enable. */
    USART2_CR1 |= (USART_CR1_UE | USART2_CR1_TE | USART2_CR1_RE);
}

/**
 * @brief   Send one byte, blocking until the transmit register is free.
 * @param   c Byte to send.
 */
void uart_write_char(char c)
{
    /* TXE means DR has been copied into the shift register, so the next byte
     * can be queued while the current one is still on the wire. */
    while (!(USART2_SR_TXE & USART2_SR));

    USART2_DR = (uint32_t)c;    /* Writing DR clears TXE in hardware. */
}

/**
 * @brief   Send a NUL-terminated string.
 * @param   str String to send; must not be NULL.
 */
void uart_print(const char *str)
{
    while (*str != '\0') {
        uart_write_char(*str);
        str++;
    }
}
