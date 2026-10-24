#include "blackpill_stm32f411.h"
#include "uart.h"

#define UART2_TX     2U
#define UART2_RX     3U
#define PCLK1_FREQ   50000000U
#define TARGET_BAUD  115200U

void uart2_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_USART2EN;

    GPIOA_MODER &= ~((0x03U << GPIO_PIN_MODE_SHIFT(UART2_TX)) | ( 0x03U << GPIO_PIN_MODE_SHIFT(UART2_RX)));
    GPIOA_MODER |= ((0x02U << GPIO_PIN_MODE_SHIFT(UART2_TX)) | (0x02U << GPIO_PIN_MODE_SHIFT(UART2_RX)));

    GPIOA_AFRL &= ~((0xFU << 8U) | (0xFU << 12U));
    GPIOA_AFRL |= ((0x7U << 8U) | (0x7U << 12U));

    uint32_t brr_value = (PCLK1_FREQ + (TARGET_BAUD / 2U)) / TARGET_BAUD;
    USART2_BRR = brr_value; 

    USART2_CR1 |= (USART_CR1_UE | USART2_CR1_TE | USART2_CR1_RE);
}

void uart_write_char(char c){
    while(!(USART2_SR_TXE & USART2_SR));
    USART2_DR = c;      // Send the character if the TX Buff is Empty
}

void uart_print(char *str){
    while(*str != '\0'){
        uart_write_char(*str);
        str++;
    }
}