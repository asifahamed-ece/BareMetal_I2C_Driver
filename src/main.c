/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Asif Ahamed S
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include <blackpill_stm32f411.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

// Defining Pins Used
#define SDA1_PIN 7U // PB7
#define SCL1_PIN 6U // PB6

const uint8_t control_byte = 0x00U;		// Tells OLED Driver that a Command upcoming
const uint8_t data_byte = 0x40U;		//Tels OLED that Data is Upcoming
#define OLED_ADDR 0x3CU  // SSD1306 OLED ADDRESS

// Forward Declarations
void I2C1_Init(void);
void I2C1_Write_Byte(uint8_t addr, uint8_t ctrl, uint8_t data);
void I2C1_Write_Buffer(uint8_t addr, uint8_t ctrl, uint8_t *buffer, uint16_t length);
void SSD1306_Cmd(uint8_t cmd);
void SSD1306_Data(uint8_t data);
void SSD1306_Init(void);
void SSD1306_SetCursor(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void SSD1306_ClearBuffer(void);
void SSD1306_Refresh(void);
void SysTick_Init(void);
void SysTick_Delay_ms(uint32_t ms);

void I2C1_Init(){

//	Enable RCC Clocks for Port B and I2C1
	RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

//	Set Modes for the Pins
	GPIOB_MODER &= ~( (0x3U << GPIO_PIN_MODE_SHIFT(6U)) | (0x3U << GPIO_PIN_MODE_SHIFT(7)) );  // Clear Mode Bits
	GPIOB_MODER |= ( (0x2U << GPIO_PIN_MODE_SHIFT(6U)) | (0x2U << GPIO_PIN_MODE_SHIFT(7)) );  // AF (10)

// Set PB6 and PB7 to Open-Drain (1)
	GPIOB_OTYPER |= ( (1 << 6U) | (1 << 7U) );

//	Set the I2C Pins to High Speed Mode (11)
	GPIOB_OSPEEDR |= ((0x03U << GPIO_PIN_MODE_SHIFT(6U)) | (0x03U << GPIO_PIN_MODE_SHIFT(7U)));

// Set Internal Pullup Resistor for I2C Lines  (01)

	GPIOB_PUPDR &= ~((0x3U << (6 * 2)) | (0x3U << (7 * 2))); // Clear existing PUPDR
	// Setting as Pull-UP
	GPIOB_PUPDR |= ( (0x01U << GPIO_PIN_MODE_SHIFT(6U)) | (0x01U << GPIO_PIN_MODE_SHIFT(7U)) );

//	Setting Up Alternative Function Registers { 20 and 24 for Pins 6 and 7 on AFRL}
	GPIOB_AFRL &= ~( ( 0xFU << 24) | (0xFU << 28) );
	GPIOB_AFRL |=  ( ( 0x4U << 24) | (0x4U << 28) );

	// I2C peripheral Configuration //

	I2C1_CR1 &= ~I2C_CR1_PE;    // Disabling I2C for Safely Configuring it.

	I2C1_CR2 &= ~(0x3FU); 	// Clearning the Last 5 bits for Freq Selection of I2C
	I2C1_CR2 |= (16U & 0x3FU);   // Setting 16MHz

	// Configure Clock Control Register (CCR) for 100 kHz Standard Mode
	// Formula: 16MHz / (2 * 100kHz) = 80
	I2C1_CCR &= ~(0xFFFU);
	I2C1_CCR |= (80U & 0xFFFU); // OxFFF => 12 Bits binary casting of 80

	// Configure Rise Time Register (TRISE)
	    // Formula: 16 MHz + 1 = 17
	I2C1_TRISE &= ~(0x3FU);
	I2C1_TRISE = (17U & 0x3FU);

//	Enable ACK, so that the master can Acknowledge Incoming data from Slaves.
	I2C1_CR1 |= I2C_CR1_ACK;  // Enabling ACK (1U << 10)

	// Finally, Enable the I2C Peripheral
	I2C1_CR1 |= I2C_CR1_PE;

// Note: For Enabling Fast Mode(400KHz) :> I2C_CCR Bit 15 is set to 1, 0 for Standard Mode (100KHz)

}

// I2C WRITE - SINGLE BYTE (for commands and small data)
void I2C1_Write_Byte(uint8_t Address, uint8_t ctrl_byte, uint8_t data) {
    I2C1_CR1 |= I2C_CR1_START;
    while (!(I2C1_SR1 & I2C_SR1_SB));

    I2C1_DR = ((Address << 1) | 0);
    while (!(I2C1_SR1 & I2C_SR1_ADDR));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    I2C1_DR = ctrl_byte;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    I2C1_DR = data;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    I2C1_CR1 |= I2C_CR1_STOP;
}

// ================================
// FRAMEBUFFER ARCHITECTURE
// =================================

// 1. The Framebuffer (1024 bytes = 128 * 64 pixels)
uint8_t SSD1306_Buffer[1024] = {0};

// 2. Burst I2C Write: Sends multiple bytes in a single transaction
void I2C1_Write_Buffer(uint8_t Dev_Address, uint8_t control_byte, uint8_t *buffer, uint16_t length) {
    // 1. Generate START
    I2C1_CR1 |= I2C_CR1_START;
    while (!(I2C1_SR1 & I2C_SR1_SB));

    // 2. Send Address
    I2C1_DR = ((Dev_Address << 1) | 0);
    while (!(I2C1_SR1 & I2C_SR1_ADDR));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    // 3. Send Control Byte
    I2C1_DR = control_byte;
    while (!(I2C1_SR1 & I2C_SR1_TXE));

    // 4. BURST DATA LOOP
    for (uint16_t i = 0; i < length; i++) {
        I2C1_DR = buffer[i];
        while (!(I2C1_SR1 & I2C_SR1_TXE)); // Wait for hardware to shift out the byte
    }

    // 5. Generate STOP
    I2C1_CR1 |= I2C_CR1_STOP;
}

// 3. Draw a single pixel into the local RAM buffer
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= 128 || y >= 64) return; // Boundary check

    // Calculate the index in the 1D array
    uint16_t index = x + (y / 8) * 128;
    uint8_t bit = y % 8;

    if (color) {
        SSD1306_Buffer[index] |= (1 << bit);  // Set the bit to 1 (White)
    } else {
        SSD1306_Buffer[index] &= ~(1 << bit); // Clear the bit to 0 (Black)
    }
}

// 4. Clear the local buffer (Fill with black)
void SSD1306_ClearBuffer(void) {
    for (int i = 0; i < 1024; i++) {
        SSD1306_Buffer[i] = 0;
    }
}

// Wrapper to send COMMANDS (Control byte = 0x00)
void SSD1306_Cmd(uint8_t cmd) {
    I2C1_Write_Byte(OLED_ADDR, 0x00U, cmd);
}

// Wrapper to send PIXEL DATA (Data byte = 0x40)
void SSD1306_Data(uint8_t data) {
    I2C1_Write_Byte(OLED_ADDR, 0x40U, data);
}

// Function to tell the display WHERE to start drawing pixels
void SSD1306_SetCursor(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end) {
    SSD1306_Cmd(0x21); // Set Column Address Command
    SSD1306_Cmd(col_start);
    SSD1306_Cmd(col_end);

    SSD1306_Cmd(0x22); // Set Page Address Command
    SSD1306_Cmd(page_start);
    SSD1306_Cmd(page_end);
}

// 5. Refresh the display (Dump the entire buffer to the OLED in ONE transaction)
void SSD1306_Refresh(void) {
    SSD1306_SetCursor(0, 127, 0, 7); // Reset drawing coordinates to top-left
    I2C1_Write_Buffer(OLED_ADDR, 0x40, SSD1306_Buffer, 1024); // Burst transfer!
}

void SSD1306_Init(void) {
    SSD1306_Cmd(0xAE); // Display OFF (Safety first)

    SSD1306_Cmd(0xD5); // Set Display Clock Divide Ratio / OSC Frequency
    SSD1306_Cmd(0x80); // Default value

    SSD1306_Cmd(0xA8); // Set Multiplex Ratio
    SSD1306_Cmd(0x3F); // 1/64 duty (for 64-pixel tall screen)

    SSD1306_Cmd(0xD3); // Set Display Offset
    SSD1306_Cmd(0x00); // No offset

    SSD1306_Cmd(0x40); // Set Display Start Line to 0

    // ⚡ THE MAGIC BULLET: Enable Charge Pump ⚡
    SSD1306_Cmd(0x8D); // Charge Pump Setting Command
    SSD1306_Cmd(0x14); // 0x14 = Enable Charge Pump (Generates the 9V needed for pixels!)

    SSD1306_Cmd(0x20); // Set Memory Addressing Mode
    SSD1306_Cmd(0x00); // Horizontal Addressing Mode (like reading a book)

    SSD1306_Cmd(0xA1); // Set Segment Re-map (Column 127 mapped to SEG0)
    SSD1306_Cmd(0xC8); // Set COM Output Scan Direction (Remapped mode)

    SSD1306_Cmd(0xDA); // Set COM Pins Hardware Configuration
    SSD1306_Cmd(0x12); // Alternative COM pin configuration

    SSD1306_Cmd(0x81); // Set Contrast Control
    SSD1306_Cmd(0xCF); // High contrast

    SSD1306_Cmd(0xD9); // Set Pre-charge Period
    SSD1306_Cmd(0xF1);

    SSD1306_Cmd(0xDB); // Set VCOMH Deselect Level
    SSD1306_Cmd(0x40);

    SSD1306_Cmd(0xA4); // Entire Display ON (Resume to RAM content)
    SSD1306_Cmd(0xA6); // Set Normal Display (Not inverted)

    SSD1306_Cmd(0xAF); // DISPLAY ON!
 }

// SysTick Timer Function for Creating Precise Delays
void SysTick_Init(void){
	// 1. Stop the Timer (Defensive coding)
	STK_CSR = 0;  // All Bits to 0  or STK_CSR &= ~(1U << 0)  : Bit 0 is Enable/Disable

    // 2. Set the Reload Value to 15999 for a 1ms delay at 16MHz CPU Clock
	STK_RVR = 15999;  // 16k ticks for 1 ms

	// 3. Clear the Current Value Reg
	STK_CVR = 0;  // This will reset the CVR

	// 4. Enable the Control and Status Reg
	STK_CSR |= ( STK_CSR_ENABLE | STK_CSR_CLKSOURCE);  // Enable the CLK Source (b2) and Timer (b0)
	// SYSTICKINT is Not enabled yet. we dont need the Interrupts yet.
}

void SysTick_Delay_ms(uint32_t ms){
	while(ms--){

		while( !(STK_CSR & STK_CSR_COUNTFLAG ));  // Wait until the Flag sets to 1  :> Rem we have used ! operator
	}
}

int main(void)
{
    // Initialize your previous peripherals
    SysTick_Init();
    I2C1_Init();

    // Give the OLED hardware 100ms to stabilize power
    SysTick_Delay_ms(100);

    // Wake up the display
    SSD1306_Init();

    // 1. Clear the local RAM buffer
    SSD1306_ClearBuffer();

    // 2. Draw a Border around the screen
    for (uint8_t x = 0; x < 128; x++) {
        SSD1306_DrawPixel(x, 0, 1);   // Top edge
        SSD1306_DrawPixel(x, 63, 1);  // Bottom edge
        SSD1306_Refresh();
        SysTick_Delay_ms(5);
    }
    for (uint8_t y = 0; y < 64; y++) {
        SSD1306_DrawPixel(0, y, 1);   // Left edge
        SSD1306_DrawPixel(127, y, 1); // Right edge
        SSD1306_Refresh();
        SysTick_Delay_ms(5);
    }

    // 3. Draw a Diagonal Line from both Sides Simultaneously..

    uint8_t j = 63;
    for (uint8_t i = 0; i < 64; i++) {
        SSD1306_DrawPixel(i * 2, i, 1);
        SSD1306_DrawPixel(i * 2, j, 1);
        SSD1306_Refresh();            // Show each new pixel
        SysTick_Delay_ms(10);
        j--;
    }

    // 4. Push the entire buffer to the OLED in ONE lightning-fast I2C burst
    SSD1306_Refresh();

    while(1) {
        // CPU is now completely free!
    }
}
