#include <stdint.h>
#include "i2c.h"

#define OLED_ADDR 0x3CU
#define OLED_CMD  0x00U
#define OLED_DATA 0x40U

// A helper to send a single command
void SSD1306_Cmd(uint8_t cmd) {
    uint8_t buf[2] = { OLED_CMD, cmd };
    i2c_write(OLED_ADDR, buf, 2);
}

// Send the full init sequence
void SSD1306_Init(void) {
    SSD1306_Cmd(0xAE);   // Display off
    SSD1306_Cmd(0xD5); SSD1306_Cmd(0x80);
    SSD1306_Cmd(0xA8); SSD1306_Cmd(0x3F);
    SSD1306_Cmd(0xD3); SSD1306_Cmd(0x00);
    SSD1306_Cmd(0x40);
    SSD1306_Cmd(0x8D); SSD1306_Cmd(0x14); // Charge pump ON
    SSD1306_Cmd(0x20); SSD1306_Cmd(0x00); // Horizontal addressing
    SSD1306_Cmd(0xA1);
    SSD1306_Cmd(0xC8);
    SSD1306_Cmd(0xDA); SSD1306_Cmd(0x12);
    SSD1306_Cmd(0x81); SSD1306_Cmd(0xCF);
    SSD1306_Cmd(0xD9); SSD1306_Cmd(0xF1);
    SSD1306_Cmd(0xDB); SSD1306_Cmd(0x40);
    SSD1306_Cmd(0xA4);
    SSD1306_Cmd(0xA6);
    SSD1306_Cmd(0xAF);   // Display ON
}

// Fill the entire screen with a specific color byte (0xFF = white, 0x00 = black)
void OLED_Fill(uint8_t color) {
    uint8_t buffer[129];
    buffer[0] = 0x40; // Control byte: 0x40 means "Data" (pixel colors)
    
    for (int i = 1; i < 129; i++) {
        buffer[i] = color;
    }

    // The screen has 8 pages (rows of 8 pixels). We write 128 bytes to each page.
    for (uint8_t page = 0; page < 8; page++) {
        // Set the column and page address window
        uint8_t cmd[] = { 0x00, 0x21, 0, 127, 0x22, page, page };
        i2c_write(0x3C, cmd, sizeof(cmd));
        
        // Send the pixel data
        i2c_write(0x3C, buffer, 129);
    }
}