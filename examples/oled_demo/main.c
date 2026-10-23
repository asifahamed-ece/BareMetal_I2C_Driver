/**
 * @file    main.c
 * @author  Asif Ahamed S
 * @brief   SSD1306 demo over a polling I2C1 master.
 *
 * Reference implementation kept for Phase 2. Not part of the default build.
 *
 * @note The SSD1306 interface is write-only. There is no read path into its
 *       GDDRAM or registers, so the display keeps a shadow framebuffer in
 *       MCU RAM and pushes it out in one burst.
 */

#include <stdint.h>
#include <blackpill_stm32f411.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SDA1_PIN 7U          ///< PB7
#define SCL1_PIN 6U          ///< PB6

const uint8_t control_byte = 0x00U;   ///< Command stream follows
const uint8_t data_byte = 0x40U;      ///< Pixel data stream follows
#define OLED_ADDR 0x3CU               ///< SSD1306 7-bit address

/** @brief Configure PB6/PB7 as I2C1 at 100 kHz, 16 MHz PCLK1. */
void I2C1_Init(void);

/**
 * @brief Write one byte after a control byte.
 * @param addr 7-bit device address.
 * @param ctrl 0x00 for commands, 0x40 for data.
 * @param data Byte to send.
 */
void I2C1_Write_Byte(uint8_t addr, uint8_t ctrl, uint8_t data);

/**
 * @brief Burst write a buffer in one transaction.
 * @param addr 7-bit device address.
 * @param ctrl 0x00 for commands, 0x40 for data.
 * @param buffer Source bytes.
 * @param length Byte count.
 */
void I2C1_Write_Buffer(uint8_t addr, uint8_t ctrl, uint8_t *buffer, uint16_t length);

void SSD1306_Cmd(uint8_t cmd);
void SSD1306_Data(uint8_t data);
void SSD1306_Init(void);

/**
 * @brief Set the drawing window.
 * @param col_start First column (0–127).
 * @param col_end   Last column.
 * @param page_start First page (0–7).
 * @param page_end   Last page.
 */
void SSD1306_SetCursor(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end);

/**
 * @brief Draw one pixel into the shadow framebuffer.
 * @param x Column, 0–127.
 * @param y Row, 0–63.
 * @param color 0 = black, non-zero = white.
 */
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

void SSD1306_ClearBuffer(void);
void SSD1306_Refresh(void);
void SysTick_Init(void);
void SysTick_Delay_ms(uint32_t ms);

void I2C1_Init(){

	/* Clocks first — registers are dead until these are set. */
	RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

	/* PB6/PB7: alternate function (10) */
	GPIOB_MODER &= ~( (0x3U << GPIO_PIN_MODE_SHIFT(6U)) | (0x3U << GPIO_PIN_MODE_SHIFT(7)) );
	GPIOB_MODER |= ( (0x2U << GPIO_PIN_MODE_SHIFT(6U)) | (0x2U << GPIO_PIN_MODE_SHIFT(7)) );

	/* Open-drain is mandatory on I2C: the bus is only ever pulled low. */
	GPIOB_OTYPER |= ( (1 << 6U) | (1 << 7U) );

	GPIOB_OSPEEDR |= ((0x03U << GPIO_PIN_MODE_SHIFT(6U)) | (0x03U << GPIO_PIN_MODE_SHIFT(7U)));

	/* Internal pull-ups as a fallback; external 4.7k is still expected. */
	GPIOB_PUPDR &= ~((0x3U << (6 * 2)) | (0x3U << (7 * 2)));
	GPIOB_PUPDR |= ( (0x01U << GPIO_PIN_MODE_SHIFT(6U)) | (0x01U << GPIO_PIN_MODE_SHIFT(7U)) );

	/* AF4 = I2C1 (datasheet Table 9). AFRL bits 27:24 = PB6, 31:28 = PB7. */
	GPIOB_AFRL &= ~( (0xFU << 24) | (0xFU << 28) );
	GPIOB_AFRL |=  ( (0x4U << 24) | (0x4U << 28) );

	/* PE must be clear while configuring. */
	I2C1_CR1 &= ~I2C_CR1_PE;

	/* FREQ = PCLK1 in MHz, 2–50. */
	I2C1_CR2 &= ~(0x3FU);
	I2C1_CR2 |= (16U & 0x3FU);

	/* Standard mode 100 kHz: CCR = PCLK1 / (2 * 100k) = 16M / 200k = 80. */
	I2C1_CCR &= ~(0xFFFU);
	I2C1_CCR |= (80U & 0xFFFU);

	/* TRISE = (max rise time / period) + 1 = 1000ns / 62.5ns + 1 = 17. */
	I2C1_TRISE &= ~(0x3FU);
	I2C1_TRISE = (17U & 0x3FU);

	I2C1_CR1 |= I2C_CR1_ACK;

	I2C1_CR1 |= I2C_CR1_PE;
}

/**
 * @note No timeout here. Phase 1 replaces this with a bounded wait that
 *       returns a status code instead of hanging on a missing device.
 */
void I2C1_Write_Byte(uint8_t Address, uint8_t ctrl_byte, uint8_t data) {
    I2C1_CR1 |= I2C_CR1_START;
    while (!(I2C1_SR1 & I2C_SR1_SB));

    I2C1_DR = ((Address << 1) | 0);
    while (!(I2C1_SR1 & I2C_SR1_ADDR));
    (void)I2C1_SR1;              /* Clear ADDR: read SR1 then SR2. */
    (void)I2C1_SR2;

    I2C1_DR = ctrl_byte;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    I2C1_DR = data;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    I2C1_CR1 |= I2C_CR1_STOP;
}

/** @brief 128x64 pixels, 1 bit each. */
uint8_t SSD1306_Buffer[1024] = {0};

void I2C1_Write_Buffer(uint8_t Dev_Address, uint8_t control_byte, uint8_t *buffer, uint16_t length) {
    I2C1_CR1 |= I2C_CR1_START;
    while (!(I2C1_SR1 & I2C_SR1_SB));

    I2C1_DR = ((Dev_Address << 1) | 0);
    while (!(I2C1_SR1 & I2C_SR1_ADDR));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    I2C1_DR = control_byte;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    for (uint16_t i = 0; i < length; i++) {
        I2C1_DR = buffer[i];
        while (!(I2C1_SR1 & I2C_SR1_TXE));
    }

    I2C1_CR1 |= I2C_CR1_STOP;
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= 128 || y >= 64) return;

    /* Page-major layout: 8 rows per byte, one byte per column. */
    uint16_t index = x + (y / 8) * 128;
    uint8_t bit = y % 8;

    if (color) {
        SSD1306_Buffer[index] |= (1 << bit);
    } else {
        SSD1306_Buffer[index] &= ~(1 << bit);
    }
}

void SSD1306_ClearBuffer(void) {
    for (int i = 0; i < 1024; i++) {
        SSD1306_Buffer[i] = 0;
    }
}

void SSD1306_Cmd(uint8_t cmd) {
    I2C1_Write_Byte(OLED_ADDR, 0x00U, cmd);
}

void SSD1306_Data(uint8_t data) {
    I2C1_Write_Byte(OLED_ADDR, 0x40U, data);
}

void SSD1306_SetCursor(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end) {
    SSD1306_Cmd(0x21);
    SSD1306_Cmd(col_start);
    SSD1306_Cmd(col_end);

    SSD1306_Cmd(0x22);
    SSD1306_Cmd(page_start);
    SSD1306_Cmd(page_end);
}

void SSD1306_Refresh(void) {
    SSD1306_SetCursor(0, 127, 0, 7);
    I2C1_Write_Buffer(OLED_ADDR, 0x40, SSD1306_Buffer, 1024);
}

/**
 * @brief SSD1306 power-on sequence.
 *
 * @note 0x8D/0x14 enables the charge pump. Without it the panel stays dark:
 *       the driver needs ~9 V for the pixels and there is no external supply.
 */
void SSD1306_Init(void) {
    SSD1306_Cmd(0xAE);   // Display off
    SSD1306_Cmd(0xD5);   // Clock divide / oscillator frequency
    SSD1306_Cmd(0x80);
    SSD1306_Cmd(0xA8);   // Multiplex ratio
    SSD1306_Cmd(0x3F);   // 1/64 duty
    SSD1306_Cmd(0xD3);   // Display offset
    SSD1306_Cmd(0x00);
    SSD1306_Cmd(0x40);   // Start line 0
    SSD1306_Cmd(0x8D);   // Charge pump
    SSD1306_Cmd(0x14);   // Enable
    SSD1306_Cmd(0x20);   // Addressing mode
    SSD1306_Cmd(0x00);   // Horizontal
    SSD1306_Cmd(0xA1);   // Segment remap
    SSD1306_Cmd(0xC8);   // COM scan direction, remapped
    SSD1306_Cmd(0xDA);   // COM pins
    SSD1306_Cmd(0x12);
    SSD1306_Cmd(0x81);   // Contrast
    SSD1306_Cmd(0xCF);
    SSD1306_Cmd(0xD9);   // Pre-charge period
    SSD1306_Cmd(0xF1);
    SSD1306_Cmd(0xDB);   // VCOMH deselect level
    SSD1306_Cmd(0x40);
    SSD1306_Cmd(0xA4);   // Resume from RAM content
    SSD1306_Cmd(0xA6);   // Normal, not inverted
    SSD1306_Cmd(0xAF);   // Display on
 }

void SysTick_Init(void){
	STK_CSR = 0;
	STK_RVR = 15999;                              /* 1 ms at 16 MHz */
	STK_CVR = 0;
	STK_CSR |= ( STK_CSR_ENABLE | STK_CSR_CLKSOURCE );
	/* TICKINT stays clear: no SysTick_Handler exists yet. */
}

void SysTick_Delay_ms(uint32_t ms){
	while(ms > 0){
		while( !(STK_CSR & STK_CSR_COUNTFLAG));
		ms--;
	}
}

int main(void)
{
    SysTick_Init();
    I2C1_Init();

    SysTick_Delay_ms(100);   /* Let the panel's supply settle. */
    SSD1306_Init();

    SSD1306_ClearBuffer();

    for (uint8_t x = 0; x < 128; x++) {
        SSD1306_DrawPixel(x, 0, 1);
        SSD1306_DrawPixel(x, 63, 1);
        SSD1306_Refresh();
        SysTick_Delay_ms(5);
    }
    for (uint8_t y = 0; y < 64; y++) {
        SSD1306_DrawPixel(0, y, 1);
        SSD1306_DrawPixel(127, y, 1);
        SSD1306_Refresh();
        SysTick_Delay_ms(5);
    }

    /* Two diagonals, drawn towards each other. */
    uint8_t j = 63;
    for (uint8_t i = 0; i < 64; i++) {
        SSD1306_DrawPixel(i * 2, i, 1);
        SSD1306_DrawPixel(i * 2, j, 1);
        SSD1306_Refresh();
        SysTick_Delay_ms(10);
        j--;
    }

    SSD1306_Refresh();

    while(1) {
    }
}
