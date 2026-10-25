/**
 * @file    ssd1306.h
 * @brief   Minimal SSD1306 128x64 OLED driver over the I2C1 master.
 *
 * The panel is not memory-mapped: every pixel write is bytes on the I2C bus,
 * at 100 kHz roughly 10 KB/s, so a full screen refresh takes ~100 ms. That is
 * the real reason this driver is command-then-blit rather than pixel-at-a-time.
 *
 * Datasheet: Solomon Systech SSD1306, sections 8 (commands) and 10 (I2C).
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

/** @brief 7-bit I2C address of the panel (0x78 in 8-bit write form). */
#define SSD1306_I2C_ADDR 0x3CU

/**
 * @brief   Run the panel's power-on initialisation sequence.
 * @note    Includes enabling the on-chip charge pump (0x8D, 0x14), without
 *          which a 3.3 V-only module stays dark no matter what else is sent.
 *          Give the panel ~100 ms after power-up before calling this.
 */
void SSD1306_Init(void);

/**
 * @brief   Fill the whole panel with one byte.
 * @param   color 0xFF = all pixels on, 0x00 = all pixels off. Any other value
 *          gives a vertical stripe pattern, since each bit is one row of a
 *          8-pixel page.
 *
 * @note    Writes 8 pages x 128 columns = 1024 bytes of GDDRAM, i.e. the
 *          entire frame buffer, one page at a time.
 */
void OLED_Fill(uint8_t color);

#endif /* SSD1306_H */
