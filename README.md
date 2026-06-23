[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![STM32F411](https://img.shields.io/badge/MCU-STM32F411CE-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f411.html)
[![Protocol](https://img.shields.io/badge/Protocol-I2C-orange.svg)](https://en.wikipedia.org/wiki/I%C2%B2C)

A **from-scratch, register-level** I2C driver for SSD1306 OLED displays on STM32F411 Black Pill. No HAL.

## 🎯 Project Overview

Complete I2C master implementation driving a 128×64 SSD1306 OLED display using framebuffer architecture

## ✨ Features

✅ **Complete I2C Master Implementation**
- Manual register configuration (no HAL)
- START/STOP condition generation
- 7-bit addressing with ACK/NACK handling
- 100kHz Standard Mode

✅ **SSD1306 OLED Driver**
- Full initialization sequence (25 commands)
- Command/Data transmission protocol
- Horizontal addressing mode
- Charge pump configuration

✅ **Framebuffer Graphics System**
- 128×64 monochrome buffer (1024 bytes)
- Pixel-level drawing API
- Optimized burst transfer (1024 bytes in 1 I2C transaction)
- 100× faster than naive byte-by-byte approach

✅ **Graphics Primitives**
- `SSD1306_DrawPixel()` - Single pixel control
- Border drawing
- Diagonal line rendering
- Expandable architecture for text and shapes

## 📊 Technical Specifications

| Component           | Specification                   |
|---------------------|---------------------------------|
| **MCU**             | STM32F411CEU6 (ARM Cortex-M4F)  |
| **Clock**           | 16 MHz HSI                      |
| **I2C Speed**       | 100 kHz (Standard Mode)         |
| **Display**         | SSD1306 0.96" OLED (128×64)     |
| **Protocol**        | I2C (7-bit addressing)          |
| **Transfer Mode**   | Burst (1024 bytes/transaction)  |
| **Performance**     | ~10ms full-screen refresh       |

**## 🔧 Hardware Setup**

### Pin Configuration

| STM32 Pin | Function | OLED Pin |
|-----------|----------|----------|
| **PB6**   | I2C1_SCL | SCL      |
| **PB7**   | I2C1_SDA | SDA      |
| **3.3V**  | Power    | VCC      |
| **GND**   | Ground   | GND      |


### Wiring

STM32F411 Black Pill         SSD1306 OLED
______________________       ____________
                      |      |           |
| PB6 ----------------+------+ SCL       |
| PB7 ----------------+------+ SDA       |
| 3.3V ---------------+------+ VCC       |
| GND ----------------+------+ GND       |
|_____________________|      |___________|


### Framebuffer Memory Mapping

The 128×64 display is organized into **8 pages** (8 rows each):

- Page 0: Bytes 0-127 (Rows 0-7)
- Page 1: Bytes 128-255 (Rows 8-15)
- Page 2: Bytes 256-383 (Rows 16-23)
- Page 3: Bytes 384-511 (Rows 24-31)
- Page 4: Bytes 512-639 (Rows 32-39)
- Page 5: Bytes 640-767 (Rows 40-47)
- Page 6: Bytes 768-895 (Rows 48-55)
- Page 7: Bytes 896-1023 (Rows 56-63)

**Pixel Mapping Formula:**
Pixel (x, y) → Buffer[x + (y/8) × 128], bit (y%8)
Example: Pixel (50, 20)
Page: 20 / 8 = 2
Bit: 20 % 8 = 4
Index: 50 + (2 × 128) = 306
Location: Buffer[306], bit 4


****🎓 Key Learnings****
**Critical I2C Concepts**

Open-Drain is mandatory - Prevents bus contention when multiple devices share the line

ADDR flag clearing trap - Must read SR1 then SR2 to clear, otherwise bus locks up

Address shifting - 7-bit address must be shifted left and OR'd with R/W bit

Pull-up resistors - Required for I2C idle state (internal or external)


**SSD1306 Controller Commands**
0xAE - Display OFF
0xAF - Display ON
0x8D - Charge pump enable (critical!)
0x20 - Memory addressing mode
0x21 - Set column address
0x22 - Set page address

**Common Pitfalls Avoided**

❌ Forgetting to clear ADDR flag → Bus lockup

❌ Using Push-Pull instead of Open-Drain → Bus contention

❌ Missing pull-up resistors → Floating bus lines

❌ Not waiting for TXE flag → Data corruption

❌ Wrong memory mapping → Garbage display


**📂 Project Structure**
STM32-I2C-OLED-BareMetal/
├── src/
│   └── main.c                 # Main application + graphics
├── include/
│   └── blackpill_stm32f411.h  # Register definitions
└── README.md   

****🔗 Resources****

[![STM32: Reference Manual]https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xc-e-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

[![SSDOLED: Datasheet]https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf?spm=a2ty_o01.29997173.0.0.794a55fb5y1dI0&file=SSD1306.pdf

[![OOLED UserGuide]https://www.nxp.com/docs/en/user-guide/UM10204.pdf?spm=a2ty_o01.29997173.0.0.794a55fb5y1dI0&file=UM10204.pdf


**👨‍ Author**

**Asif Ahamed S**

\Final Year - Electronics & Communication Engineering

Rajalakshmi Engineering College, Thiruvallur

🔗 LinkedIn: linkedin.com/in/asif-ahamed-s-ece

📧 Email: asifahamed670@gmail.com
