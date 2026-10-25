#include "blackpill_stm32f411.h"
#include "i2c.h"
#include "timebase.h"

#define SCL_PIN     6U  // PB6 
#define SDA_PIN     7U  // PB7
#define PCLK1       50000000U   // 50MHz

void i2c_init()
{
    // Init the i2c_2 pins of Blackpill and Set their modes
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB_MODER &= ~((0x3U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x3U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));
    GPIOB_MODER |= ((0x2U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x2U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));

    // Set PB6 and PB7 to Pull-Up mode
    GPIOB_PUPDR &= ~((0x3U << GPIO_PIN_MODE_SHIFT(6U)) | (0x3U << GPIO_PIN_MODE_SHIFT(7U)));
    GPIOB_PUPDR |=  ((0x1U << GPIO_PIN_MODE_SHIFT(6U)) | (0x1U << GPIO_PIN_MODE_SHIFT(7U)));

    GPIOB_OTYPER |= ((1U << SCL_PIN) | (1U << SDA_PIN));

    GPIOB_OSPEEDR |= ((0x3U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x3U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));

    GPIOB_AFRL &= ~((0xFU << (SCL_PIN * 4U)) | (0xFU << (SDA_PIN * 4U)));
    GPIOB_AFRL |= ((0x4U << (SCL_PIN * 4U) | (0x4U << (SDA_PIN * 4U))));

    // I2C Core Configurations
    I2C1_CR1 &= ~I2C_CR1_PE;        
    // Ensure I2C is Disabled beforew Configs
    I2C1_CR1 |= I2C_CR1_SWRST; I2C1_CR1 &= ~I2C_CR1_SWRST;

    I2C1_CR2 = ((I2C1_CR2 & ~0x3FU) | 50U);    // CLK Freq to 50MHz
    uint32_t CCR_val = PCLK1 / (2 * 100000);

    I2C1_CCR &= ~(0xFFFU); I2C1_CCR |= (CCR_val << 0U);

    uint32_t Risetime_val = (PCLK1 / 1000000) + 1;
    I2C1_TRISE &= ~(0x3FU << 0U); I2C1_TRISE |= (Risetime_val << 0U);    // 50+1 for 50MHz Clock Line
    
    I2C1_CR1 |= I2C_CR1_ACK;    // Enable the I2C Acknowledge 
    // Enable the Peripheral (I2C1)
    I2C1_CR1 |= I2C_CR1_PE;
}

uint8_t i2c_probe(uint8_t addr) {
    // 1. Wait for BUSY flag to clear
    uint32_t start = timebase_us();
    while(I2C1_SR2 & (1U << 1U)) {
        if((timebase_us() - start) > 2000) return 0;
    }

    // 2. Generate START
    I2C1_CR1 |= I2C_CR1_START;

    // 3. Wait for SB flag
    start = timebase_us();
    while(!(I2C1_SR1 & I2C_SR1_SB)) {
        if((timebase_us() - start) > 2000) return 0;
    }

    // 4. Send Address
    I2C1_DR = (addr << 1) | 0;

    // 5. WAIT for ADDR (ACK) or AF (NACK) flag!
    start = timebase_us();
    while(!(I2C1_SR1 & I2C_SR1_ADDR) && !(I2C1_SR1 & I2C1_SR1_ACK_FAILURE)) {
        if((timebase_us() - start) > 2000) return 0;
    }

    // 6. Check what happened
    uint8_t found = 0;
    if(I2C1_SR1 & I2C_SR1_ADDR) {
        found = 1;
        (void)I2C1_SR2; // Clear ADDR flag by reading SR2
    } else {
        I2C1_SR1 &= ~I2C1_SR1_ACK_FAILURE; // Clear AF flag
    }

    // 7. Generate STOP!
    I2C1_CR1 |= I2C_CR1_STOP;

    return found;
}