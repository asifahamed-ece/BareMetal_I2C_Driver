/**
 * @file    uart.h
 * @brief   Baremetal USART2 transmit/receive on PA2 (TX) and PA3 (RX).
 *
 * Reference: RM0383 §19 "Universal synchronous asynchronous receiver
 *            transmitter (USART)", p505.
 *
 * 115200 8N1, polling driven, no interrupts. The pins are wired to the SH1106/
 * ESP32 receiver in docs/UART_RX_ESP32; PA2 is the STM32's transmit line and
 * must never be driven by anything else.
 *
 * @warning PA2 must be wired to a real UART RX pin. On an ESP32 DevKit that
 *          is GPIO16 (UART2 RX), not the pin silkscreened "RX", which is
 *          GPIO3 (UART0 RX) and already owned by the USB bridge.
 */

#ifndef UART2_H
#define UART2_H

/** @brief Configure PA2/PA3 as USART2 at 115200 baud. Call after clock_init_100Mhz(). */
void uart2_init(void);

/**
 * @brief   Send one byte, blocking until the transmit register has room.
 * @param   c Byte to send.
 * @note    Returns as soon as DR is free, not when the frame has fully left
 *          the wire. Fine for a stream of characters; if the very next thing
 *          you do is disable USART2 or reset, wait for TC first or the last
 *          byte is truncated.
 */
void uart_write_char(char c);

/**
 * @brief   Send a NUL-terminated string.
 * @param   str String to send; must not be NULL.
 * @note    `\n` alone moves the cursor down without returning to column 0.
 *          Use `"\r\n"` for terminals.
 */
void uart_print(const char *str);

#endif /* UART2_H */
