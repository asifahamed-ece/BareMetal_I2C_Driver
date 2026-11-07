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
 * @param   len  Number of bytes to send.
 * @return  1 on success, 0 on NACK or on any timeout. A STOP is always
 *          issued before returning, so the bus is left idle either way.
 *
 * @note    The transfer ends with STOP, so this cannot be chained into a
 *          repeated START. Use i2c_read() when a register pointer has to be
 *          written before reading.
 */
uint8_t i2c_write(uint8_t addr, uint8_t *data, uint32_t len);

/**
 * @brief   Read @p len bytes starting at register @p reg of a 7-bit slave.
 * @param   addr 7-bit address (MPU6050 answers at 0x68).
 * @param   reg  Register address to read from.
 * @param   data Buffer of at least @p len bytes to receive into.
 * @param   len  Bytes to read; must be >= 1.
 * @return  1 on success, 0 on NACK or timeout. @p data is untouched on 0.
 *
 * @note    This is the combined-format transfer every register-mapped sensor
 *          expects:
 *          @verbatim
 *          START -> ADDR+W -> REG -> Sr -> ADDR+R -> DATA[0..N-1] -> STOP
 *          @endverbatim
 *          The repeated START (Sr) keeps the bus claimed, so no other master
 *          can slip a transaction between "here is the register" and "give me
 *          its contents".
 *
 * @note    ACK timing is the subtle part and is why the implementation has
 *          three branches. A slave only releases the bus when it sees a NACK,
 *          so the master must decide "one more byte?" *before* the last byte
 *          arrives (RM0383 §18.3.4). One byte, two bytes and N bytes each
 *          have a different correct answer.
 */
uint8_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);

/**
 * @brief   Attempt to free a bus that a slave is holding by clocking SCL.
 *
 * @note    A slave interrupted mid-byte keeps SDA low waiting for clocks that
 *          will never come, and every later START then fails. Recovery bit-bangs
 *          up to nine clocks so the slave can finish its byte, then emits a
 *          manual STOP and re-initialises I2C1.
 *
 * @note    The pins are re-pointed at the GPIO output driver (MODER = 0b01) and
 *          stay open-drain, so writing 1 releases the line to its pull-up and
 *          writing 0 pulls it down — the same electrical contract the peripheral
 *          uses. Nine clocks are also the most a slave can need, not a promise,
 *          which is why the outcome is reported rather than assumed.
 *
 * @return  1 if SDA reads high after the STOP (bus released), 0 if a slave is
 *          still holding it low. A 0 means the caller should treat the bus as
 *          unusable rather than retrying blind.
 */
uint8_t i2c_bus_recover(void);

#endif /* I2C_H */
