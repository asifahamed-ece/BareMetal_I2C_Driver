/**
 * @file    ssd1306.c
 * @brief   SSD1306 128x64 OLED driver: init sequence and full-screen fill.
 *
 * Datasheet: Solomon Systech SSD1306, section 8 (command table).
 *
 * Everything here is two I2C primitives. A transfer always starts with a
 * control byte that tells the panel how to read the rest of the burst:
 *   0x00  the following bytes are COMMANDS
 *   0x40  the following bytes are GDDRAM DATA (pixels)
 * Get that byte wrong and the panel silently ignores the whole transfer —
 * which looks exactly like a wiring fault.
 */

#include "ssd1306.h"
#include "i2c.h"

#define OLED_ADDR SSD1306_I2C_ADDR  ///< 0x3C, 7-bit
#define OLED_CMD  0x00U             ///< Control byte: Co = 0, D/C# = 0 -> command stream
#define OLED_DATA 0x40U             ///< Control byte: Co = 0, D/C# = 1 -> data stream

/**
 * @brief   Send one command byte to the panel.
 * @param   cmd Command from the datasheet's command table.
 *
 * @note    Commands are single bytes but must still travel as
 *          `{ control, command }`; the panel has no other way to tell a
 *          command from a pixel. Two bytes per command at 100 kHz is why a
 *          20-command init sequence visibly takes a few milliseconds.
 */
static void SSD1306_Cmd(uint8_t cmd)
{
    uint8_t buf[2] = { OLED_CMD, cmd };
    i2c_write(OLED_ADDR, buf, 2);
}

/**
 * @brief   Run the panel's initialisation sequence.
 *
 * @note    Order matters. The display is kept off (0xAE) until every
 *          parameter is set, because changing configuration on a running
 *          panel produces visible garbage rather than an error.
 */
void SSD1306_Init(void)
{
    SSD1306_Cmd(0xAE);                  /* Display OFF while we configure. */

    SSD1306_Cmd(0xD5); SSD1306_Cmd(0x80);   /* Clock divide 1, oscillator default. */
    SSD1306_Cmd(0xA8); SSD1306_Cmd(0x3F);   /* Multiplex ratio 63 -> 64 rows. */
    SSD1306_Cmd(0xD3); SSD1306_Cmd(0x00);   /* Display offset 0: COM0 to row 0. */
    SSD1306_Cmd(0x40);                      /* Start line 0. */

    /* Charge pump. Without it a 3.3 V-only module has no ~7.5 V rail for the
     * OLED segments and stays black. 0x10 disables, 0x14 enables. */
    SSD1306_Cmd(0x8D); SSD1306_Cmd(0x14);

    /* Horizontal addressing: after a data burst the column pointer wraps and
     * the page advances, so a page can be filled with one continuous write. */
    SSD1306_Cmd(0x20); SSD1306_Cmd(0x00);

    SSD1306_Cmd(0xA1);                  /* Segment remap: column 127 -> SEG0 (mirror X). */
    SSD1306_Cmd(0xC8);                  /* COM scan remapped (mirror Y). */
    SSD1306_Cmd(0xDA); SSD1306_Cmd(0x12);   /* Alternative COM pins, for 128x64. */
    SSD1306_Cmd(0x81); SSD1306_Cmd(0xCF);   /* Contrast 207. */
    SSD1306_Cmd(0xD9); SSD1306_Cmd(0xF1);   /* Pre-charge: phase 1/2 for internal pump. */
    SSD1306_Cmd(0xDB); SSD1306_Cmd(0x40);   /* VCOMH deselect ~0.83 x Vcc. */

    SSD1306_Cmd(0xA4);                  /* Resume from GDDRAM, not "all on". */
    SSD1306_Cmd(0xA6);                  /* Normal polarity, not inverted. */
    SSD1306_Cmd(0xAF);                  /* Display ON. */
}

/**
 * @brief   Fill the entire panel with one colour byte.
 * @param   color 0xFF = all on, 0x00 = all off.
 *
 * @note    GDDRAM is organised as 8 "pages" of 128 bytes. One byte covers
 *          eight vertical pixels in a single column, so a page is 8 rows tall
 *          and 128 bytes fill it exactly. 8 pages x 128 bytes = 1024 bytes,
 *          the whole panel.
 *
 * @note    The window is re-opened per page with 0x21 (column range) and
 *          0x22 (page range). Without that, the panel's internal pointer
 *          would decide where the next byte lands and the image would skew.
 */
void OLED_Fill(uint8_t color)
{
    /* 129 = one control byte plus the 128 pixel bytes of a page. Built once
     * and reused for all eight pages. */
    uint8_t buffer[129];
    buffer[0] = OLED_DATA;              /* "the next 128 bytes are pixels" */

    for (int i = 1; i < 129; i++) {
        buffer[i] = color;
    }

    for (uint8_t page = 0; page < 8; page++) {
        /* 0x21 sets the column window (start, end), 0x22 the page window.
         * Setting both to a single page makes the following data burst land
         * exactly there and nowhere else. */
        uint8_t cmd[] = { OLED_CMD, 0x21, 0x00, 0x7F, 0x22, page, page };
        i2c_write(OLED_ADDR, cmd, sizeof(cmd));

        /* One page of pixels: 1 control byte + 128 data bytes. */
        i2c_write(OLED_ADDR, buffer, 129);
    }
}
