/**
 * @file    blackpill_stm32f411.h
 * @brief   Register map for the STM32F411CEU6 (WeAct BlackPill V3.0).
 * @author  Asif Ahamed S
 *
 * Addresses and offsets verified against RM0383. No generated code, no HAL.
 *
 * @note Enable a peripheral's clock in RCC before touching its registers,
 *       and never set an interrupt-enable bit before the handler exists —
 *       unwritten handlers alias to Default_Handler, which is `b .`.
 */

#ifndef BLACKPILL_STM32F411_H
#define BLACKPILL_STM32F411_H

#include <stdint.h>

/** @name Peripheral bus base addresses
 *  RM0383 §2.3, Table 1.
 *  @{ */
#define PERIPH_BASE           ((uint32_t)0x40000000U)
#define APB1PERIPH_BASE       (PERIPH_BASE + 0x00000000U)  ///< Low-speed peripherals
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000U)  ///< High-speed peripherals
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000U)
/** @} */

/** @name RCC — reset and clock control
 *  RM0383 §6.3, Table 35.
 *  @{ */
#define RCC_BASE              ((uint32_t)0x40023800U)

#define RCC_CR                (*(volatile uint32_t*)(RCC_BASE + 0x00U))  ///< HSI/HSE/PLL enable
#define RCC_PLLCFGR           (*(volatile uint32_t*)(RCC_BASE + 0x04U))
#define RCC_CFGR              (*(volatile uint32_t*)(RCC_BASE + 0x08U))  ///< Prescalers, clock source
#define RCC_CIR               (*(volatile uint32_t*)(RCC_BASE + 0x0CU))
#define RCC_AHB1RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x10U))
#define RCC_APB1RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x20U))
#define RCC_AHB1ENR           (*(volatile uint32_t*)(RCC_BASE + 0x30U))
#define RCC_APB1ENR           (*(volatile uint32_t*)(RCC_BASE + 0x40U))
#define RCC_APB2ENR           (*(volatile uint32_t*)(RCC_BASE + 0x44U))
#define RCC_APB2RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x24U))

#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)   ///< Onboard LED is on PC13
#define RCC_AHB1ENR_GPIODEN   (1U << 3)
#define RCC_AHB1ENR_GPIOEEN   (1U << 4)
#define RCC_AHB1ENR_GPIOHEN   (1U << 7)

#define RCC_USART2EN          (1U << 17)  ///< RCC_APB1ENR bit — USART2
#define USART2_CR1_TE         (1U << 3U) 
#define USART2_CR1_RE         (1U << 2U)

#define RCC_APB1ENR_I2C1EN    (1U << 21)
#define RCC_APB1ENR_TIM3EN    (1U << 1)

#define RCC_APB2ENR_SYSCFGEN  (1U << 14)  ///< Required for EXTI pin mapping
/** @} */

/** @name PWR — power controller
 *  RM0383 §5.4. Voltage scaling must be raised before SYSCLK goes up.
 *  @{ */
#define PWR_BASE              ((uint32_t)0x40007000U)

#define PWR_CR                (*(volatile uint32_t*)(PWR_BASE + 0x00U))  ///< VOS is bits 15:14
#define PWR_CSR               (*(volatile uint32_t*)(PWR_BASE + 0x04U))
/** @} */

/** @name FLASH — embedded flash interface
 *  RM0383 §3.8. Wait states must be raised before SYSCLK exceeds the limit
 *  they currently support, or the CPU fetches garbage.
 *  @{ */
#define FLASH_BASE            ((uint32_t)0x40023C00U)

#define FLASH_ACR             (*(volatile uint32_t*)(FLASH_BASE + 0x00U))  ///< Latency is bits 3:0
#define FLASH_KEYR            (*(volatile uint32_t*)(FLASH_BASE + 0x04U))
#define FLASH_OPTKEYR         (*(volatile uint32_t*)(FLASH_BASE + 0x08U))
#define FLASH_SR              (*(volatile uint32_t*)(FLASH_BASE + 0x0CU))
#define FLASH_CR              (*(volatile uint32_t*)(FLASH_BASE + 0x10U))
#define FLASH_OPTCR           (*(volatile uint32_t*)(FLASH_BASE + 0x14U))
/** @} */

/** @name DWT — Data Watchpoint and Trace (Core Peripheral)
 *  ARMv7-M Architecture Reference Manual
 *  @{ */
#define DWT_BASE              ((uint32_t)0xE0001000U)
#define DWT_CTRL              (*(volatile uint32_t*)(DWT_BASE + 0x00U))
#define DWT_CYCCNT            (*(volatile uint32_t*)(DWT_BASE + 0x04U))

#define CoreDebug_BASE        ((uint32_t)0xE000EDF0U)
#define CoreDebug_DEMCR       (*(volatile uint32_t*)(CoreDebug_BASE + 0x0CU))

#define TRCENA               (1U << 24U)
#define CYCCNTENA             (1U << 0U)

/** @} */


/** @name GPIO — general purpose I/O
 *  RM0383 §8.4, Table 27. Every port has the same register layout.
 *  @{ */
#define GPIOA_BASE            ((uint32_t)0x40020000U)
#define GPIOB_BASE            ((uint32_t)0x40020400U)
#define GPIOC_BASE            ((uint32_t)0x40020800U)  ///< Onboard LED is on PC13
#define GPIOD_BASE            ((uint32_t)0x40020C00U)
#define GPIOE_BASE            ((uint32_t)0x40021000U)
#define GPIOH_BASE            ((uint32_t)0x40021C00U)

#define GPIOx_MODER_OFF       0x00U   ///< 2 bits per pin: 00 in, 01 out, 10 AF, 11 analog
#define GPIOx_OTYPER_OFF      0x04U   ///< 1 bit per pin: 0 push-pull, 1 open-drain
#define GPIOx_OSPEEDR_OFF     0x08U   ///< 2 bits per pin
#define GPIOx_PUPDR_OFF       0x0CU   ///< 2 bits per pin: 00 none, 01 up, 10 down
#define GPIOx_IDR_OFF         0x10U   ///< Read-only
#define GPIOx_ODR_OFF         0x14U
#define GPIOx_BSRR_OFF        0x18U   ///< Write-only, atomic: [15:0] set, [31:16] reset
#define GPIOx_LCKR_OFF        0x1CU
#define GPIOx_AFRL_OFF        0x20U   ///< AF for pins 0–7, 4 bits per pin
#define GPIOx_AFRH_OFF        0x24U   ///< AF for pins 8–15, 4 bits per pin

/** @brief GPIO port registers. RM0383 §8.3. */
typedef struct
{
    volatile uint32_t MODER;    ///< 0x00
    volatile uint32_t OTYPER;   ///< 0x04
    volatile uint32_t OSPEEDR;  ///< 0x08
    volatile uint32_t PUPDR;    ///< 0x0C
    volatile uint32_t IDR;      ///< 0x10 read-only
    volatile uint32_t ODR;      ///< 0x14
    volatile uint32_t BSRR;     ///< 0x18
    volatile uint32_t LCKR;     ///< 0x1C
    volatile uint32_t AFR[2];   ///< 0x20 AFRL, 0x24 AFRH
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD   ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE   ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH   ((GPIO_TypeDef *)GPIOH_BASE)

#define GPIOA_MODER           (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_MODER_OFF))
#define GPIOA_OTYPER          (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_OTYPER_OFF))
#define GPIOA_OSPEEDR         (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_OSPEEDR_OFF))
#define GPIOA_PUPDR           (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_PUPDR_OFF))
#define GPIOA_IDR             (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_IDR_OFF))
#define GPIOA_ODR             (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_ODR_OFF))
#define GPIOA_BSRR            (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_BSRR_OFF))

#define GPIOB_MODER           (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_MODER_OFF))
#define GPIOB_OTYPER          (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_OTYPER_OFF))
#define GPIOB_OSPEEDR         (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_OSPEEDR_OFF))
#define GPIOB_PUPDR           (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_PUPDR_OFF))
#define GPIOB_IDR             (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_IDR_OFF))
#define GPIOB_ODR             (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_ODR_OFF))
#define GPIOB_BSRR            (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_BSRR_OFF))

#define GPIOC_MODER           (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_MODER_OFF))
#define GPIOC_OTYPER          (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_OTYPER_OFF))
#define GPIOC_OSPEEDR         (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_OSPEEDR_OFF))
#define GPIOC_PUPDR           (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_PUPDR_OFF))
#define GPIOC_IDR             (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_IDR_OFF))
#define GPIOC_ODR             (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_ODR_OFF))
#define GPIOC_BSRR            (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_BSRR_OFF))

#define GPIOA_AFRL            (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_AFRL_OFF))
#define GPIOA_AFRH            (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_AFRH_OFF))
#define GPIOB_AFRL            (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_AFRL_OFF))
#define GPIOB_AFRH            (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_AFRH_OFF))
/** @} */

/** @name SYSCFG — tells EXTI which port to watch
 *  RM0383 §7.2, Table 30.
 *  @{ */
#define SYSCFG_BASE           ((uint32_t)0x40013800U)
#define SYSCFG_EXTICR1        (*(volatile uint32_t*)(SYSCFG_BASE + 0x08U))  ///< EXTI0–3
#define SYSCFG_EXTICR2        (*(volatile uint32_t*)(SYSCFG_BASE + 0x0CU))  ///< EXTI4–7
#define SYSCFG_EXTICR3        (*(volatile uint32_t*)(SYSCFG_BASE + 0x10U))  ///< EXTI8–11
#define SYSCFG_EXTICR4        (*(volatile uint32_t*)(SYSCFG_BASE + 0x14U))  ///< EXTI12–15

#define SYSCFG_EXTICR1_EXTI0_PA  (0x0U << 0)
#define SYSCFG_EXTICR1_EXTI0_PB  (0x1U << 0)
#define SYSCFG_EXTICR1_EXTI0_PC  (0x2U << 0)
#define SYSCFG_EXTICR1_EXTI0_PD  (0x3U << 0)
/** @} */

/** @name EXTI — external interrupt/event controller
 *  RM0383 §10.3, Table 70.
 *  @{ */
#define EXTI_BASE             ((uint32_t)0x40013C00U)

#define EXTI_IMR              (*(volatile uint32_t*)(EXTI_BASE + 0x00U))  ///< Interrupt mask
#define EXTI_EMR              (*(volatile uint32_t*)(EXTI_BASE + 0x04U))  ///< Event mask
#define EXTI_RTSR             (*(volatile uint32_t*)(EXTI_BASE + 0x08U))  ///< Rising edge (LOW→HIGH)
#define EXTI_FTSR             (*(volatile uint32_t*)(EXTI_BASE + 0x0CU))  ///< Falling edge (HIGH→LOW)
#define EXTI_SWIER            (*(volatile uint32_t*)(EXTI_BASE + 0x10U))  ///< Software trigger
#define EXTI_PR               (*(volatile uint32_t*)(EXTI_BASE + 0x14U))  ///< Pending, write 1 to clear

#define EXTI_LINE_0           (1U << 0)
#define EXTI_LINE_1           (1U << 1)
#define EXTI_LINE_2           (1U << 2)
#define EXTI_LINE_3           (1U << 3)
#define EXTI_LINE_4           (1U << 4)
#define EXTI_LINE_5           (1U << 5)
#define EXTI_LINE_6           (1U << 6)
#define EXTI_LINE_7           (1U << 7)
#define EXTI_LINE_8           (1U << 8)
#define EXTI_LINE_9           (1U << 9)
#define EXTI_LINE_10          (1U << 10)
#define EXTI_LINE_11          (1U << 11)
#define EXTI_LINE_12          (1U << 12)
#define EXTI_LINE_13          (1U << 13)
#define EXTI_LINE_14          (1U << 14)
#define EXTI_LINE_15          (1U << 15)
/** @} */

/** @name NVIC — nested vectored interrupt controller
 *  ARM DDI0403E §B3.4. Core peripheral, not in RM0383.
 *  @{ */
#define NVIC_BASE             ((uint32_t)0xE000E100U)

#define NVIC_ISER0            (*(volatile uint32_t*)(NVIC_BASE + 0x00U))  ///< Enable IRQ 0–31
#define NVIC_ISER1            (*(volatile uint32_t*)(NVIC_BASE + 0x04U))  ///< Enable IRQ 32–63
#define NVIC_ICER             (*(volatile uint32_t*)(NVIC_BASE + 0x80U))  ///< Disable
#define NVIC_ISPR             (*(volatile uint32_t*)(NVIC_BASE + 0x100U)) ///< Set pending
#define NVIC_ICPR             (*(volatile uint32_t*)(NVIC_BASE + 0x180U)) ///< Clear pending
#define NVIC_IPR              (*(volatile uint32_t*)(NVIC_BASE + 0x400U)) ///< Priority 0–255

/* From RM0383 Table 37. */
#define NVIC_IRQ_WWDG         0
#define NVIC_IRQ_PVD          1
#define NVIC_IRQ_TAMP_STAMP   2
#define NVIC_IRQ_RTC_WKUP     3
#define NVIC_IRQ_FLASH        4
#define NVIC_IRQ_RCC          5
#define NVIC_IRQ_EXTI0        6
#define NVIC_IRQ_EXTI1        7
#define NVIC_IRQ_EXTI2        8
#define NVIC_IRQ_EXTI3        9
#define NVIC_IRQ_EXTI4        10
#define NVIC_IRQ_EXTI9_5      23   ///< EXTI 5–9 share this IRQ
#define NVIC_IRQ_USART1       37
#define NVIC_IRQ_USART2       38
#define NVIC_IRQ_EXTI15_10    40   ///< EXTI 10–15 share this IRQ
#define NVIC_IRQ_USART6       71

#define NVIC_IRQ_BIT(irqn)    (1U << ((irqn) & 0x1FU))  ///< Bit within ISER/ICER
#define NVIC_IRQ_REG(irqn)    ((irqn) >> 5)             ///< Which ISER/ICER word
/** @} */

/** @name SysTick — Cortex-M4 system timer
 *  PM0214 §4.5. Core peripheral. 24-bit down-counter.
 *  @{ */
#define SYSTICK_BASE          ((uint32_t)0xE000E010U)

#define STK_CSR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x00U))
#define STK_RVR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x04U))  ///< 24-bit reload
#define STK_CVR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x08U))  ///< Any write clears it and COUNTFLAG
#define STK_CALIB             (*(volatile uint32_t*)(SYSTICK_BASE + 0x0CU))  ///< Read-only

#define STK_CSR_ENABLE        (1U << 0)
#define STK_CSR_TICKINT       (1U << 1)   ///< Interrupt on underflow
#define STK_CSR_CLKSOURCE     (1U << 2)   ///< 1 = processor clock, 0 = processor clock / 8
#define STK_CSR_COUNTFLAG     (1U << 16)  ///< Set on wrap; read-only, cleared by reading CSR
/** @} */

/** @name Core instructions
 *  @{ */
#define __WFI()               __asm__ volatile ("wfi")  ///< Sleep until interrupt
#define __WFE()               __asm__ volatile ("wfe")  ///< Sleep until event
#define __SEV()               __asm__ volatile ("sev")  ///< Send event
#define __DSB()               __asm__ volatile ("dsb")  ///< Data sync barrier
#define __DMB()               __asm__ volatile ("dmb")  ///< Data memory barrier
#define __ISB()               __asm__ volatile ("isb")  ///< Instruction sync barrier
/** @} */

/** @name USART — RM0383 §19.6, Table 88
 *  @{ */
#define USART1_BASE           ((uint32_t)(0x40011000U))
#define USART2_BASE           ((uint32_t)(0x40004400U))
#define USART6_BASE           ((uint32_t)(0x40011400U))

#define USART2_SR             (*(volatile uint32_t*)(USART2_BASE + 0x00U))
#define USART2_DR             (*(volatile uint32_t*)(USART2_BASE + 0x04U))
#define USART2_BRR            (*(volatile uint32_t*)(USART2_BASE + 0x08U))
#define USART2_CR1            (*(volatile uint32_t*)(USART2_BASE + 0x0CU))
#define USART2_CR2            (*(volatile uint32_t*)(USART2_BASE + 0x10U))
#define USART2_CR3            (*(volatile uint32_t*)(USART2_BASE + 0x14U))

#define USART_CR1_UE          (1U << 13)
#define USART_CR1_TE          (1U << 3)
#define USART_CR1_RE          (1U << 2)
#define USART_CR1_RXNEIE      (1U << 5)   ///< RXNE interrupt, for interrupt-driven RX

#define USART2_SR_TXE         (1U << 7)
#define USART2_SR_RXNE        (1U << 5)
/** @} */

/** @name GPIO pin helpers
 *  @{ */
#define GPIO_PIN_MODE_SHIFT(pin)    ((pin) * 2U)              ///< MODER / PUPDR / OSPEEDR
#define GPIO_BSRR_SET(pin)          (1U << (pin))             ///< Drive pin high
#define GPIO_BSRR_RST(pin)          (1U << ((pin) + 16U))     ///< Drive pin low
#define GPIO_PIN_MASK(pin)          (1U << (pin))             ///< Single-bit mask for IDR / ODR
/** @} */

/** @name ADC1 — RM0383 §12
 *  @{ */
#define ADC1_BASE             ((uint32_t)0x40012000U)

#define ADC1_SR               (*(volatile uint32_t*)(ADC1_BASE + 0x00U))
#define ADC1_CR1              (*(volatile uint32_t*)(ADC1_BASE + 0x04U))
#define ADC1_CR2              (*(volatile uint32_t*)(ADC1_BASE + 0x08U))
#define ADC1_SMPR1            (*(volatile uint32_t*)(ADC1_BASE + 0x0CU))
#define ADC1_SMPR2            (*(volatile uint32_t*)(ADC1_BASE + 0x10U))
#define ADC1_JOFR1            (*(volatile uint32_t*)(ADC1_BASE + 0x14U))  ///< Injected offset 1
#define ADC1_JOFR2            (*(volatile uint32_t*)(ADC1_BASE + 0x18U))  ///< Injected offset 2
#define ADC1_JOFR3            (*(volatile uint32_t*)(ADC1_BASE + 0x1CU))  ///< Injected offset 3
#define ADC1_JOFR4            (*(volatile uint32_t*)(ADC1_BASE + 0x20U))  ///< Injected offset 4
#define ADC1_HTR              (*(volatile uint32_t*)(ADC1_BASE + 0x24U))  ///< Watchdog high threshold
#define ADC1_LTR              (*(volatile uint32_t*)(ADC1_BASE + 0x28U))  ///< Watchdog low threshold
#define ADC1_SQR1             (*(volatile uint32_t*)(ADC1_BASE + 0x2CU))  ///< Regular sequence 1
#define ADC1_SQR2             (*(volatile uint32_t*)(ADC1_BASE + 0x30U))  ///< Regular sequence 2
#define ADC1_SQR3             (*(volatile uint32_t*)(ADC1_BASE + 0x34U))  ///< Regular sequence 3
#define ADC1_JSQR             (*(volatile uint32_t*)(ADC1_BASE + 0x38U))  ///< Injected sequence
#define ADC1_JDR1             (*(volatile uint32_t*)(ADC1_BASE + 0x3CU))  ///< Injected data 1
#define ADC1_JDR2             (*(volatile uint32_t*)(ADC1_BASE + 0x40U))  ///< Injected data 2
#define ADC1_JDR3             (*(volatile uint32_t*)(ADC1_BASE + 0x44U))  ///< Injected data 3
#define ADC1_JDR4             (*(volatile uint32_t*)(ADC1_BASE + 0x48U))  ///< Injected data 4
#define ADC1_DR               (*(volatile uint32_t*)(ADC1_BASE + 0x4CU))  ///< Regular data
#define ADC1_CCR              (*(volatile uint32_t*)(ADC1_BASE + 0x308U)) ///< Common control

#define ADC_SR_EOC            (1U << 1)
#define ADC_SR_AWD            (1U << 2)
#define ADC_SR_JEOC           (1U << 3)
#define ADC_SR_JSTRT          (1U << 4)
#define ADC_SR_STRT           (1U << 5)

#define ADC_CR1_AWDCH         (0x1FU << 0)
#define ADC_CR1_EOCIE         (1U << 5)
#define ADC_CR1_AWDIE         (1U << 6)
#define ADC_CR1_JEOCIE        (1U << 7)
#define ADC_CR1_SCAN          (1U << 8)
#define ADC_CR1_AWDSGL        (1U << 9)
#define ADC_CR1_JAUTO         (1U << 10)
#define ADC_CR1_JDISCEN       (1U << 11)
#define ADC_CR1_DISCEN        (1U << 12)
#define ADC_CR1_DISCNUM       (0x7U << 13)
#define ADC_CR1_JAWDEN        (1U << 22)
#define ADC_CR1_AWDEN         (1U << 23)
#define ADC_CR1_RES           (0x3U << 24)  ///< 00 = 12-bit
#define ADC_CR1_OVERW         (1U << 26)

#define ADC_CR2_ADON          (1U << 0)
#define ADC_CR2_CONT          (1U << 1)
#define ADC_CR2_DMA           (1U << 8)
#define ADC_CR2_DDS           (1U << 9)
#define ADC_CR2_EOCS          (1U << 10)
#define ADC_CR2_ALIGN         (1U << 11)  ///< 0 right, 1 left
#define ADC_CR2_JEXTSEL       (0xFU << 12)
#define ADC_CR2_JEXTEN        (0x3U << 16)
#define ADC_CR2_JSWSTART      (1U << 22)
#define ADC_CR2_EXTSEL        (0xFU << 24)
#define ADC_CR2_EXTEN         (0x3U << 28)
#define ADC_CR2_SWSTART       (1U << 30)

#define ADC_SMPR2_SMP0        (0x7U << 0)
#define ADC_SMPR2_SMP1        (0x7U << 3)   ///< PA1 = ADC1_IN1
#define ADC_SMPR2_SMP2        (0x7U << 6)

#define ADC_SMP_3CYCLES       0x0U
#define ADC_SMP_15CYCLES      0x1U
#define ADC_SMP_28CYCLES      0x2U
#define ADC_SMP_56CYCLES      0x3U
#define ADC_SMP_84CYCLES      0x4U
#define ADC_SMP_112CYCLES     0x5U
#define ADC_SMP_144CYCLES     0x6U
#define ADC_SMP_480CYCLES     0x7U

#define RCC_APB2ENR_ADC1EN    (1U << 8)
/** @} */

/** @name TIM3 — general purpose timer, APB1
 *  @{ */
#define TIM3_BASE             ((uint32_t)0x40000400U)

#define TIM3_CR1              (*(volatile uint32_t*)(TIM3_BASE + 0x00U))
#define TIM3_EGR              (*(volatile uint32_t*)(TIM3_BASE + 0x14U))
#define TIM3_CCMR1            (*(volatile uint32_t*)(TIM3_BASE + 0x18U))
#define TIM3_CCER             (*(volatile uint32_t*)(TIM3_BASE + 0x20U))
#define TIM3_PSC              (*(volatile uint32_t*)(TIM3_BASE + 0x28U))
#define TIM3_ARR              (*(volatile uint32_t*)(TIM3_BASE + 0x2CU))
#define TIM3_CCR1             (*(volatile uint32_t*)(TIM3_BASE + 0x34U))
/** @} */

/** @name I2C1 — inter-integrated circuit, APB1
 *  RM0383 §18. PB6 = SCL, PB7 = SDA, both AF4.
 *  @{ */
#define I2C1_BASE             ((uint32_t)0x40005400U)

#define I2C1_CR1              (*(volatile uint32_t*)(I2C1_BASE + 0x00U))
#define I2C1_CR2              (*(volatile uint32_t*)(I2C1_BASE + 0x04U))  ///< Contains FREQ (bits 5:0)
#define I2C1_OAR1             (*(volatile uint32_t*)(I2C1_BASE + 0x08U))
#define I2C1_DR               (*(volatile uint32_t*)(I2C1_BASE + 0x10U))
#define I2C1_SR1              (*(volatile uint32_t*)(I2C1_BASE + 0x14U))
#define I2C1_SR2              (*(volatile uint32_t*)(I2C1_BASE + 0x18U))  ///< Reading clears ADDR
#define I2C1_CCR              (*(volatile uint32_t*)(I2C1_BASE + 0x1CU))
#define I2C1_TRISE            (*(volatile uint32_t*)(I2C1_BASE + 0x20U))

#define I2C_CR1_PE            (1U << 0)
#define I2C_CR1_START         (1U << 8)
#define I2C_CR1_STOP          (1U << 9)
#define I2C_CR1_ACK           (1U << 10)
#define I2C_CR1_SWRST         (1U << 15)

#define I2C_SR1_SB            (1U << 0)   ///< START generated
#define I2C_SR1_ADDR          (1U << 1)   ///< Address sent/matched; clear by reading SR1 then SR2
#define I2C_SR1_TXE           (1U << 7)   ///< Data register empty
#define I2C1_SR1_ACK_FAILURE  (1U << 10)  ///< NACK received
#define I2C1_SR1_BTF          (1U << 2)   ///< Byte Transfer Completed Flag
#define I2C1_SR1_BUSY         (1U << 1)   ///< I2C is BUSY Mid-Transmission

/** @} */


/** @name PLL and clock-tree configuration
 *  RM0383 §6.3. The PLL chain is:
 *    HSI / PLLM = VCO input (1-2 MHz)
 *    VCO input x PLLN = VCO output (100-432 MHz)
 *    VCO output / PLLP = SYSCLK
 *    VCO output / PLLQ = 48 MHz domain
 *  @{ */
#define RCC_APB1ENR_PWREN      (1U << 28U)       ///< PWR clock; needed before writing PWR_CR
#define VOS_Scale1             (0x03U << 14U)    ///< Scale 1, required above 84 MHz
#define VOS_Scale2             (0x02U << 14U)    ///< Scale 2, up to 84 MHz
#define VOS_Scale3             (0x01U << 14U)    ///< Scale 3, up to 16 MHz

#define FLASH_LATENCY_3WAITS   (0x03U << 0U)     ///< 3 wait states, 90-100 MHz at 2.7-3.6 V
#define FLASH_PRFTEN           (1U << 8U)        ///< Prefetch
#define FLASH_ICEN             (1U << 9U)        ///< Instruction cache
#define FLASH_DCEN             (1U << 10U)       ///< Data cache

#define RCC_PLLSRC_HSE         (0x01U << 22U)    ///< PLL input = HSE
#define RCC_PLLSRC_HSI         (0x00U << 22U)    ///< PLL input = HSI

#define RCC_PLLCFGR_PLLM_Pos   0U                ///< Bits 5:0, VCO input divider
#define RCC_PLLCFGR_PLLM_Msk   (0x3FU << RCC_PLLCFGR_PLLM_Pos)

#define RCC_PLLCFGR_PLLN_Pos   6U                ///< Bits 14:6, VCO multiplier
#define RCC_PLLCFGR_PLLN_Msk   (0x1FFU << RCC_PLLCFGR_PLLN_Pos)

#define RCC_PLLCFGR_PLLP_Pos   16U               ///< Bits 17:16, 00 = /2, 01 = /4, 10 = /6, 11 = /8
#define RCC_PLLCFGR_PLLP_Msk   (0x3U << RCC_PLLCFGR_PLLP_Pos)

#define RCC_PLLCFGR_PLLSRC_Pos 22U               ///< 0 = HSI, 1 = HSE
#define RCC_PLLCFGR_PLLSRC_Msk (0x1U << RCC_PLLCFGR_PLLSRC_Pos)

#define RCC_PLLCFGR_PLLQ_Pos   24U               ///< Bits 27:24, 48 MHz domain divider
#define RCC_PLLCFGR_PLLQ_Msk   (0xFU << RCC_PLLCFGR_PLLQ_Pos)

#define RCC_CR_PLL_ON          (1U << 24U)       ///< PLL enable
#define RCC_CR_PLL_RDY         (1U << 25U)       ///< Set once the PLL has locked

#define RCC_CFGR_HSI_SYSCLK    (0x00U << 0U)     ///< SW = HSI
#define RCC_CFGR_HSE_SYSCLE    (0x01U << 0U)     ///< SW = HSE. @bug name typo, should be HSE_SYSCLK

/** @brief SW = PLL.
 *  @bug Value is 0x10, but SW is bits 1:0 — 0x10 sets HPRE bit 4 as well and
 *       would silently halve AHB. Should be (0x02U << 0U). */
#define RCC_CFGR_PLL_SYSCLK    (0x10U << 0U)

/** @brief Mask for the SWS field. @bug SWS is bits 3:2; only bit 2 is
 *         covered here. Should be (0x3U << 2U). */
#define RCC_CLK_SWS            (1U << 2U)
/** @} */


#endif /* BLACKPILL_STM32F411_H */
