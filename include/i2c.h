/**
 * @file    i2c.h
 * @brief   Baremetal I2C1 master for STM32F411 (SCL on PB6, SDA on PB7).
 *
 * Reference: RM0383 §18 "Inter-integrated circuit (I2C) interface", p472;
 *            register map §18.6, p496.
 *
 * Standard mode (100 kHz), 7-bit addressing, polling driven: every wait is a
 * loop on a status flag with a timeout. No interrupts, no DMA. That makes the
 * driver easy to follow at the cost of blocking the caller while a transfer
 * runs — fine for a sensor or an OLED, wrong for a fast bus.
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/**
 * @brief   Bring I2C1 up as a 100 kHz master on PB6/PB7.
 *
 * @note    Call once, after clock_init_100Mhz(). CCR and TRISE are derived
 *          from PCLK1 = 50 MHz, so a different APB1 clock needs different
 *          divisors — this driver does not measure the bus for you.
 */
void i2c_init(void);

/**
 * @brief   Probe one 7-bit address: START, address, STOP.
 * @param   addr 7-bit address in the range 0x08..0x77.
 * @return  1 if some device acknowledged, 0 if nothing answered or a flag
 *          never arrived within the timeout.
 *
 * @note    A NACK is a legitimate answer, not a fault: it is the bus saying
 *          "no device owns this address". A *timeout* is the suspicious one —
 *          it means SCL or SDA stayed stuck, usually a missing pull-up or a
 *          device holding SDA low.
 */
uint8_t i2c_probe(uint8_t addr);

/**
 * @brief   Write @p len bytes from @p data to a 7-bit addressed slave.
 * @param   addr 7-bit address (SSD1306 answers at 0x3C).
 * @param   data Buffer to send; `data[0]` goes on the wire first.
 * @param   len  Number of bytes to send (max 255).
 * @return  1 on success, 0 on NACK or on any timeout. A STOP is always
 *          issued before returning, so the bus is left idle either way.
 *
 * @warning Write only — no repeated START and no read path yet.
 */
uint8_t i2c_write(uint8_t addr, uint8_t *data, uint8_t len);

#endif /* I2C_H */
