/*
 * blackpill_stm32f411.h
 *
 * Custom hardware abstraction header for WeAct BlackPill V3.0 (STM32F411CEU6)
 * All addresses/offsets extracted from RM0383 Reference Manual
 *
 * Created on: 31-May-2026
 * Author: Asif Ahamed S
 */

#ifndef BLACKPILL_STM32F411_H
#define BLACKPILL_STM32F411_H

#include <stdint.h>

/* ========================================================================
 * MEMORY MAP & BUS BASE ADDRESSES
 * Source: RM0383 Section 2.3 "Memory organization" Table 1
 * These define the start of each peripheral bus region in the 4GB address space
 * ======================================================================== */
#define PERIPH_BASE           ((uint32_t)0x40000000U)  // Start of all peripheral memory region
#define APB1PERIPH_BASE       (PERIPH_BASE + 0x00000000U)  // APB1 bus base (low-speed peripherals)
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000U)  // APB2 bus base (high-speed peripherals)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000U)  // AHB1 bus base (core system peripherals)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000U)  // AHB2 bus base (USB, RNG, etc.)

/* ========================================================================
 * RCC - Reset & Clock Control
 * Source: RM0383 Section 6.3, Table 35 "RCC registers"
 * Controls clock gating for all peripherals - MUST enable before using any peripheral
 * ======================================================================== */
#define RCC_BASE              ((uint32_t)0x40023800U)  // RCC peripheral base address (AHB1 bus)

// RCC Register Pointers (Base + Offset)
#define RCC_CR                (*(volatile uint32_t*)(RCC_BASE + 0x00U))  // Clock control register (HSI/HSE/PLL enable)
#define RCC_PLLCFGR           (*(volatile uint32_t*)(RCC_BASE + 0x04U))  // PLL configuration register
#define RCC_CFGR              (*(volatile uint32_t*)(RCC_BASE + 0x08U))  // Clock configuration register (prescalers, clock source)
#define RCC_CIR               (*(volatile uint32_t*)(RCC_BASE + 0x0CU))  // Clock interrupt register
#define RCC_AHB1ENR           (*(volatile uint32_t*)(RCC_BASE + 0x30U))  // AHB1 peripheral clock enable register
#define RCC_AHB1RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x10U))  // AHB1 peripheral reset register
#define RCC_APB1ENR           (*(volatile uint32_t*)(RCC_BASE + 0x40U))  // APB1 peripheral clock enable register (I2C, USART2, etc)
#define RCC_APB1RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x20U))  // APB1 peripheral reset register
#define RCC_APB2ENR           (*(volatile uint32_t*)(RCC_BASE + 0x44U))  // APB2 peripheral clock enable register (SYSCFG, SPI, USART1, USART6, etc)
#define RCC_APB2RSTR          (*(volatile uint32_t*)(RCC_BASE + 0x24U))  // APB2 peripheral reset register


// RCC_AHB1ENR Bit Masks - Set these bits to ENABLE clock for corresponding peripheral
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)  // Enable GPIOA clock (bit 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)  // Enable GPIOB clock (bit 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)  // Enable GPIOC clock (bit 2) ← Used for PC13 LED
#define RCC_AHB1ENR_GPIODEN   (1U << 3)  // Enable GPIOD clock (bit 3)
#define RCC_AHB1ENR_GPIOEEN   (1U << 4)  // Enable GPIOE clock (bit 4)
#define RCC_AHB1ENR_GPIOHEN   (1U << 7)  // Enable GPIOH clock (bit 7)

// RCC_USART Bit MAsk
#define RCC_USART2EN          (1U << 17) // Enable USAERT2 CLock (Bit 17)

// RCC_APB2ENR Bit Masks
#define RCC_APB2ENR_SYSCFGEN  (1U << 14) // Enable SYSCFG clock (bit 14) ← Required for EXTI pin mapping

/* ========================================================================
 * GPIO PORTS - General Purpose Input/Output
 * Source: RM0383 Section 8.4, Table 27 "GPIO registers"
 * Each port (A-H) has identical register layout at different base addresses
 * ======================================================================== */
// GPIO Port Base Addresses (from RM0383 Table 1)
#define GPIOA_BASE            ((uint32_t)0x40020000U)  // GPIO Port A base address
#define GPIOB_BASE            ((uint32_t)0x40020400U)  // GPIO Port B base address
#define GPIOC_BASE            ((uint32_t)0x40020800U)  // GPIO Port C base address ← Used for PC13 LED
#define GPIOD_BASE            ((uint32_t)0x40020C00U)  // GPIO Port D base address
#define GPIOE_BASE            ((uint32_t)0x40021000U)  // GPIO Port E base address
#define GPIOH_BASE            ((uint32_t)0x40021C00U)  // GPIO Port H base address

// GPIO Register Offsets (relative to GPIOx_BASE) - Same for all ports
#define GPIOx_MODER_OFF       0x00U  // Mode register offset: configures pin as input/output/AF/analog
#define GPIOx_OTYPER_OFF      0x04U  // Output type register offset: push-pull (0) or open-drain (1)
#define GPIOx_OSPEEDR_OFF     0x08U  // Output speed register offset: configures slew rate
#define GPIOx_PUPDR_OFF       0x0CU  // Pull-up/pull-down register offset: configures internal resistors
#define GPIOx_IDR_OFF         0x10U  // Input data register offset: READ-ONLY, reflects pin voltage
#define GPIOx_ODR_OFF         0x14U  // Output data register offset: software-controlled output value
#define GPIOx_BSRR_OFF        0x18U  // Bit set/reset register offset: ATOMIC pin toggle (write-only)
#define GPIOx_LCKR_OFF        0x1CU  // Lock register offset: prevents accidental pin reconfiguration
#define GPIOx_AFRL_OFF        0x20U  // Alternate function low register offset: AF config for pins 0-7
#define GPIOx_AFRH_OFF        0x24U  // Alternate function high register offset: AF config for pins 8-15

// GPIO Register Pointers for GPIOA (PA0-PA15)
#define GPIOA_MODER           (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_MODER_OFF))  // PA pin mode control
#define GPIOA_OTYPER          (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_OTYPER_OFF)) // PA output type control
#define GPIOA_OSPEEDR         (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_OSPEEDR_OFF))// PA output speed control
#define GPIOA_PUPDR           (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_PUPDR_OFF))  // PA pull-up/pull-down control
#define GPIOA_IDR             (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_IDR_OFF))    // PA input data (READ-ONLY)
#define GPIOA_ODR             (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_ODR_OFF))    // PA output data control
#define GPIOA_BSRR            (*(volatile uint32_t*)(GPIOA_BASE + GPIOx_BSRR_OFF))   // PA atomic set/reset

// GPIO Register Pointers for GPIOA (PA0-PA15)
#define GPIOB_MODER           (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_MODER_OFF))  // PB pin mode control
#define GPIOB_OTYPER          (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_OTYPER_OFF)) // PB output type control
#define GPIOB_OSPEEDR         (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_OSPEEDR_OFF))// PB output speed control
#define GPIOB_PUPDR           (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_PUPDR_OFF))  // PB pull-up/pull-down control
#define GPIOB_IDR             (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_IDR_OFF))    // PB input data (READ-ONLY)
#define GPIOB_ODR             (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_ODR_OFF))    // PB output data control
#define GPIOB_BSRR            (*(volatile uint32_t*)(GPIOB_BASE + GPIOx_BSRR_OFF))   // PB atomic set/reset

// GPIO Register Pointers for GPIOC (PC0-PC15)
#define GPIOC_MODER           (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_MODER_OFF))  // PC pin mode control
#define GPIOC_OTYPER          (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_OTYPER_OFF)) // PC output type control
#define GPIOC_OSPEEDR         (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_OSPEEDR_OFF))// PC output speed control
#define GPIOC_PUPDR           (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_PUPDR_OFF))  // PC pull-up/pull-down control
#define GPIOC_IDR             (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_IDR_OFF))    // PC input data (READ-ONLY)
#define GPIOC_ODR             (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_ODR_OFF))    // PC output data control
#define GPIOC_BSRR            (*(volatile uint32_t*)(GPIOC_BASE + GPIOx_BSRR_OFF))   // PC atomic set/reset

// Alternate Function Registers
#define GPIOA_AFRL           (*(volatile uint32_t*)(GPIOA_BASE + 0x20U))  // PA pin Alt Fn Control (GPIO Pins 0 to 7)
#define GPIOA_AFRH 			 (*(volatile uint32_t*)(GPIOA_BASE + 0x24U))  // For Pins 8 to 15
#define GPIOB_AFRL           (*(volatile uint32_t*)(GPIOB_BASE + 0x20U))  // PA pin Alt Fn Control (GPIO Pins 0 to 7)
#define GPIOB_AFRH 			 (*(volatile uint32_t*)(GPIOB_BASE + 0x24U))  // For Pins 8 to 15

/* ========================================================================
 * SYSCFG - System Configuration Controller
 * Source: RM0383 Section 7.2, Table 30 "SYSCFG registers"
 * Required for EXTI pin mapping (tells EXTI which GPIO port to watch)
 * ======================================================================== */
#define SYSCFG_BASE           ((uint32_t)0x40013800U)  // SYSCFG peripheral base address (APB2 bus)
#define SYSCFG_EXTICR1        (*(volatile uint32_t*)(SYSCFG_BASE + 0x08U))  // EXTI0-3 pin mapping register
#define SYSCFG_EXTICR2        (*(volatile uint32_t*)(SYSCFG_BASE + 0x0CU))  // EXTI4-7 pin mapping register
#define SYSCFG_EXTICR3        (*(volatile uint32_t*)(SYSCFG_BASE + 0x10U))  // EXTI8-11 pin mapping register
#define SYSCFG_EXTICR4        (*(volatile uint32_t*)(SYSCFG_BASE + 0x14U))  // EXTI12-15 pin mapping register

// SYSCFG_EXTICR1 Bit Field Values for EXTI0 mapping (bits [3:0])
#define SYSCFG_EXTICR1_EXTI0_PA  (0x0U << 0)  // Map EXTI0 to PA0
#define SYSCFG_EXTICR1_EXTI0_PB  (0x1U << 0)  // Map EXTI0 to PB0
#define SYSCFG_EXTICR1_EXTI0_PC  (0x2U << 0)  // Map EXTI0 to PC0
#define SYSCFG_EXTICR1_EXTI0_PD  (0x3U << 0)  // Map EXTI0 to PD0

/* ========================================================================
 * EXTI - External Interrupt/Event Controller
 * Source: RM0383 Section 10.3, Table 70 "EXTI registers"
 * Detects edges on GPIO pins and signals NVIC to trigger interrupt
 * ======================================================================== */
#define EXTI_BASE             ((uint32_t)0x40013C00U)  // EXTI peripheral base address (APB2 bus)

// EXTI Register Pointers (Base + Offset)
#define EXTI_IMR              (*(volatile uint32_t*)(EXTI_BASE + 0x00U))  // Interrupt mask register: enable/disable EXTI lines
#define EXTI_EMR              (*(volatile uint32_t*)(EXTI_BASE + 0x04U))  // Event mask register: enable/disable event generation
#define EXTI_RTSR             (*(volatile uint32_t*)(EXTI_BASE + 0x08U))  // Rising trigger selection: detect HIGH→LOW edges
#define EXTI_FTSR             (*(volatile uint32_t*)(EXTI_BASE + 0x0CU))  // Falling trigger selection: detect LOW→HIGH edges
#define EXTI_SWIER            (*(volatile uint32_t*)(EXTI_BASE + 0x10U))  // Software interrupt event register: trigger EXTI manually
#define EXTI_PR               (*(volatile uint32_t*)(EXTI_BASE + 0x14U))  // Pending register: shows which lines triggered (write-1-to-clear)

// EXTI Line Bit Masks (for IMR, FTSR, PR, etc.)
#define EXTI_LINE_0           (1U << 0)   // Bit mask for EXTI Line 0 (PA0/PB0/PC0...)
#define EXTI_LINE_1           (1U << 1)   // Bit mask for EXTI Line 1
#define EXTI_LINE_2           (1U << 2)   // Bit mask for EXTI Line 2
#define EXTI_LINE_3           (1U << 3)   // Bit mask for EXTI Line 3
#define EXTI_LINE_4           (1U << 4)   // Bit mask for EXTI Line 4
#define EXTI_LINE_5           (1U << 5)   // Bit mask for EXTI Line 5
#define EXTI_LINE_6           (1U << 6)   // Bit mask for EXTI Line 6
#define EXTI_LINE_7           (1U << 7)   // Bit mask for EXTI Line 7
#define EXTI_LINE_8           (1U << 8)   // Bit mask for EXTI Line 8
#define EXTI_LINE_9           (1U << 9)   // Bit mask for EXTI Line 9
#define EXTI_LINE_10          (1U << 10)  // Bit mask for EXTI Line 10
#define EXTI_LINE_11          (1U << 11)  // Bit mask for EXTI Line 11
#define EXTI_LINE_12          (1U << 12)  // Bit mask for EXTI Line 12
#define EXTI_LINE_13          (1U << 13)  // Bit mask for EXTI Line 13
#define EXTI_LINE_14          (1U << 14)  // Bit mask for EXTI Line 14
#define EXTI_LINE_15          (1U << 15)  // Bit mask for EXTI Line 15

/* ========================================================================
 * NVIC - Nested Vectored Interrupt Controller
 * Source: ARM DDI0403E Section B3.4, Table B3-8 "NVIC registers"
 * ARM Core peripheral: manages interrupt priorities and dispatch to ISRs
 * ======================================================================== */
#define NVIC_BASE             ((uint32_t)0xE000E100U)  // NVIC base address (Core peripheral, not in RM0383)

// NVIC Register Pointers
#define NVIC_ISER0             (*(volatile uint32_t*)(NVIC_BASE + 0x00U))  // Interrupt Set-Enable Register: enable IRQ by setting bit { For IRQs 0 to 31}
#define NVIC_ISER1            (*(volatile uint32_t*)(NVIC_BASE + 0x04U))  // IRQs 32-63
#define NVIC_ISER1            (*(volatile uint32_t*)(NVIC_BASE + 0x04U))  // IRQs 34-95
#define NVIC_ICER             (*(volatile uint32_t*)(NVIC_BASE + 0x80U))  // Interrupt Clear-Enable Register: disable IRQ by setting bit
#define NVIC_ISPR             (*(volatile uint32_t*)(NVIC_BASE + 0x100U)) // Interrupt Set-Pending Register: manually trigger IRQ
#define NVIC_ICPR             (*(volatile uint32_t*)(NVIC_BASE + 0x180U)) // Interrupt Clear-Pending Register: clear pending IRQ
#define NVIC_IPR              (*(volatile uint32_t*)(NVIC_BASE + 0x400U)) // Interrupt Priority Registers: set priority 0-255

// NVIC IRQ Numbers for STM32F411 (from RM0383 Table 37 "Vector table")
// Used with NVIC_ISER: NVIC_ISER |= (1U << IRQ_NUMBER)
#define NVIC_IRQ_WWDG         0   // Window Watchdog interrupt
#define NVIC_IRQ_PVD          1   // PVD through EXTI line detection
#define NVIC_IRQ_TAMP_STAMP   2   // Tamper and TimeStamp interrupts
#define NVIC_IRQ_RTC_WKUP     3   // RTC Wakeup interrupt
#define NVIC_IRQ_FLASH        4   // FLASH global interrupt
#define NVIC_IRQ_RCC          5   // RCC global interrupt
#define NVIC_IRQ_EXTI0        6   // EXTI Line 0 interrupt ← Used for PA0 button
#define NVIC_IRQ_EXTI1        7   // EXTI Line 1 interrupt
#define NVIC_IRQ_EXTI2        8   // EXTI Line 2 interrupt
#define NVIC_IRQ_EXTI3        9   // EXTI Line 3 interrupt
#define NVIC_IRQ_EXTI4        10  // EXTI Line 4 interrupt
#define NVIC_IRQ_EXTI9_5      23  // EXTI Lines 9-5 shared interrupt
#define NVIC_IRQ_EXTI15_10    40  // EXTI Lines 15-10 shared interrupt
#define NVIC_IRQ_USART2       38  // USART2 global interrupt
#define NVIC_IRQ_USART1       37  // USART1 global interrupt
#define NVIC_IRQ_USART6       71  // USART6 global interrupt

// Helper Macros for NVIC bit manipulation
#define NVIC_IRQ_BIT(irqn)    (1U << ((irqn) & 0x1FU))  // Calculate bit position in ISER/ICER for given IRQ number
#define NVIC_IRQ_REG(irqn)    ((irqn) >> 5)             // Calculate which ISER register (0-7) for IRQ > 31

/* ========================================================================
 * SYSTICK - Cortex-M4 System Timer
 * Source: ARM DDI0403E Section B3.3 "SysTick Timer"
 * Core peripheral: provides precise timing, RTOS tick, delays
 * ======================================================================== */
#define SYSTICK_BASE          ((uint32_t)0xE000E010U)  // SysTick base address (Core peripheral)

// SysTick Register Pointers
#define STK_CSR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x00U))  // Control and Status register
#define STK_RVR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x04U))  // Reload Value register (24-bit)
#define STK_CVR               (*(volatile uint32_t*)(SYSTICK_BASE + 0x08U))  // Current Value register (read-only, clears on read)
#define STK_CALIB             (*(volatile uint32_t*)(SYSTICK_BASE + 0x0CU))  // Calibration Value register (read-only)

// STK_CSR Bit Masks
#define STK_CSR_ENABLE        (1U << 0)  // Bit 0: Enable SysTick counter (1=enabled, 0=disabled)
#define STK_CSR_TICKINT       (1U << 1)  // Bit 1: Enable SysTick interrupt (1=interrupt on underflow)
#define STK_CSR_CLKSOURCE     (1U << 2)  // Bit 2: Clock source (1=processor clock, 0=external reference)
#define STK_CSR_COUNTFLAG     (1U << 16) // Bit 16: Count flag (set when counter reaches 0, read-only)

/* ========================================================================
 * CORE UTILITY MACROS
 * ARM Cortex-M4 assembly instructions for power management and memory ordering
 * ======================================================================== */
#define __WFI()               __asm__ volatile ("wfi")  // Wait For Interrupt: puts CPU to sleep until interrupt occurs
#define __WFE()               __asm__ volatile ("wfe")  // Wait For Event: sleep until event signal (SEV) received
#define __SEV()               __asm__ volatile ("sev")  // Send Event: wakes all CPUs waiting on WFE
#define __DSB()               __asm__ volatile ("dsb")  // Data Synchronization Barrier: ensures memory accesses complete
#define __DMB()               __asm__ volatile ("dmb")  // Data Memory Barrier: ensures memory access ordering
#define __ISB()               __asm__ volatile ("isb")  // Instruction Synchronization Barrier: flushes pipeline after config changes


/* ========================================================================
 * USART Register Addresses and Offsets
 * Source: RM0383 Section 19.6  :  Table 88
 * ======================================================================== */
#define USART1_BASE 			((uint32_t)(0x40011000U))
#define USART2_BASE 			((uint32_t)(0x40004400U))
#define USART6_BASE 			((uint32_t)(0x40011400U))

/* USART2-Specific Register Pointers */
#define USART2_SR         (*(volatile uint32_t*)(USART2_BASE + 0x00U))
#define USART2_DR         (*(volatile uint32_t*)(USART2_BASE + 0x04U))
#define USART2_BRR        (*(volatile uint32_t*)(USART2_BASE + 0x08U))
#define USART2_CR1        (*(volatile uint32_t*)(USART2_BASE + 0x0CU))
#define USART2_CR2        (*(volatile uint32_t*)(USART2_BASE + 0x10U))
#define USART2_CR3        (*(volatile uint32_t*)(USART2_BASE + 0x14U))

/* USART_CR1 Bit Masks (RM0383 Section 19.6.1) */
#define USART_CR1_UE      (1U << 13)  // USART Enable
#define USART_CR1_TE      (1U << 3)   // Transmitter Enable
#define USART_CR1_RE      (1U << 2)   // Receiver Enable
#define USART_CR1_RXNEIE  (1U << 5)   // RXNE Interrupt Enable (for Interrupt Driven Non-Blocking Rx)

// Status Bits
#define USART2_SR_TXE         (1U << 7)   // Transmit Data Register Empty
#define USART2_SR_RXNE        (1U << 5)   // Receive Data Register Not Empty

/* ========================================================================
 * GPIO PIN HELPER MACROS (Optional but for better readability)
 * Calculate bit positions for a given pin number in GPIO registers
 * ======================================================================== */
// MODER/PUPDR/OSPEEDR: each pin uses 2 bits, position = pin_number * 2
#define GPIO_PIN_MODE_SHIFT(pin)    ((pin) * 2U)  // Calculate bit shift for pin in MODER/PUPDR/OSPEEDR

// BSRR: bits [15:0] set pin HIGH, bits [31:16] reset pin LOW
#define GPIO_BSRR_SET(pin)          (1U << (pin))        // BSRR value to SET pin HIGH
#define GPIO_BSRR_RST(pin)        (1U << ((pin) + 16U)) // BSRR value to RESET pin LOW

// IDR/ODR: each pin uses 1 bit at position = pin_number
#define GPIO_PIN_MASK(pin)          (1U << (pin))  // Bit mask to read/write single pin in IDR/ODR

/* ========================================================================
 * ADC1 - Analog to Digital Converter
 * Source: RM0383 Section 12 "Analog-to-digital converter (ADC)"
 * ======================================================================== */
#define ADC1_BASE             ((uint32_t)0x40012000U)  // ADC1 base address (APB2 bus)

// ADC Register Pointers (Base + Offset)
#define ADC1_SR               (*(volatile uint32_t*)(ADC1_BASE + 0x00U))  // Status register
#define ADC1_CR1              (*(volatile uint32_t*)(ADC1_BASE + 0x04U))  // Control register 1
#define ADC1_CR2              (*(volatile uint32_t*)(ADC1_BASE + 0x08U))  // Control register 2
#define ADC1_SMPR1            (*(volatile uint32_t*)(ADC1_BASE + 0x0CU))  // Sample time register 1
#define ADC1_SMPR2            (*(volatile uint32_t*)(ADC1_BASE + 0x10U))  // Sample time register 2
#define ADC1_JOFR1            (*(volatile uint32_t*)(ADC1_BASE + 0x14U))  // Injected channel data offset
#define ADC1_JOFR2            (*(volatile uint32_t*)(ADC1_BASE + 0x18U))  // Injected channel data offset
#define ADC1_JOFR3            (*(volatile uint32_t*)(ADC1_BASE + 0x1CU))  // Injected channel data offset
#define ADC1_JOFR4            (*(volatile uint32_t*)(ADC1_BASE + 0x20U))  // Injected channel data offset
#define ADC1_HTR              (*(volatile uint32_t*)(ADC1_BASE + 0x24U))  // Watchdog higher threshold
#define ADC1_LTR              (*(volatile uint32_t*)(ADC1_BASE + 0x28U))  // Watchdog lower threshold
#define ADC1_SQR1             (*(volatile uint32_t*)(ADC1_BASE + 0x2CU))  // Regular sequence register 1
#define ADC1_SQR2             (*(volatile uint32_t*)(ADC1_BASE + 0x30U))  // Regular sequence register 2
#define ADC1_SQR3             (*(volatile uint32_t*)(ADC1_BASE + 0x34U))  // Regular sequence register 3
#define ADC1_JSQR             (*(volatile uint32_t*)(ADC1_BASE + 0x38U))  // Injected sequence register
#define ADC1_JDR1             (*(volatile uint32_t*)(ADC1_BASE + 0x3CU))  // Injected data register 1
#define ADC1_JDR2             (*(volatile uint32_t*)(ADC1_BASE + 0x40U))  // Injected data register 2
#define ADC1_JDR3             (*(volatile uint32_t*)(ADC1_BASE + 0x44U))  // Injected data register 3
#define ADC1_JDR4             (*(volatile uint32_t*)(ADC1_BASE + 0x48U))  // Injected data register 4
#define ADC1_DR               (*(volatile uint32_t*)(ADC1_BASE + 0x4CU))  // Regular data register
#define ADC1_CCR              (*(volatile uint32_t*)(ADC1_BASE + 0x308U)) // Common control register (for calibration)

// ADC_SR Bit Masks (Status Register)
#define ADC_SR_EOC            (1U << 1)   // End of Conversion flag (bit 1)
#define ADC_SR_AWD            (1U << 2)   // Analog watchdog flag
#define ADC_SR_JEOC           (1U << 3)   // Injected channel end of conversion
#define ADC_SR_JSTRT          (1U << 4)   // Injected channel Start flag
#define ADC_SR_STRT           (1U << 5)   // Regular channel Start flag

// ADC_CR1 Bit Masks  (Control Register 1)
#define ADC_CR1_AWDCH         (0x1FU << 0)  // Analog watchdog channel select
#define ADC_CR1_EOCIE         (1U << 5)   // Interrupt enable for EOC
#define ADC_CR1_AWDIE         (1U << 6)   // Analog watchdog interrupt enable
#define ADC_CR1_JEOCIE        (1U << 7)   // Interrupt enable for injected channels
#define ADC_CR1_SCAN          (1U << 8)   // Scan mode
#define ADC_CR1_AWDSGL        (1U << 9)   // Enable the watchdog on a single channel
#define ADC_CR1_JAUTO         (1U << 10)  // Automatic injected group conversion
#define ADC_CR1_JDISCEN       (1U << 11)  // Discontinuous mode on injected channels
#define ADC_CR1_DISCEN        (1U << 12)  // Discontinuous mode on regular channels
#define ADC_CR1_DISCNUM       (0x7U << 13) // Discontinuous mode channel count
#define ADC_CR1_JAWDEN        (1U << 22)  // Analog watchdog enable on injected channels
#define ADC_CR1_AWDEN         (1U << 23)  // Analog watchdog enable on regular channels
#define ADC_CR1_RES           (0x3U << 24) // Resolution (12-bit = 00)
#define ADC_CR1_OVERW         (1U << 26)  // Overrun mode

// ADC_CR2 Bit Masks	(Control Register 2)
#define ADC_CR2_ADON          (1U << 0)   // A/D Converter ON / OFF
#define ADC_CR2_CONT          (1U << 1)   // Continuous Conversion
#define ADC_CR2_DMA           (1U << 8)   // Direct memory access mode
#define ADC_CR2_DDS           (1U << 9)   // DMA disable selection
#define ADC_CR2_EOCS          (1U << 10)  // End of conversion selection
#define ADC_CR2_ALIGN         (1U << 11)  // Data alignment (0=right, 1=left)
#define ADC_CR2_JEXTSEL       (0xFU << 12) // External trigger select for injected
#define ADC_CR2_JEXTEN        (0x3U << 16) // External trigger enable for injected
#define ADC_CR2_JSWSTART      (1U << 22)  // Start conversion of injected channels
#define ADC_CR2_EXTSEL        (0xFU << 24) // External trigger select for regular
#define ADC_CR2_EXTEN         (0x3U << 28) // External trigger enable for regular
#define ADC_CR2_SWSTART       (1U << 30)  // Start conversion of regular channels

// ADC_SMPR2 Bit Masks (for channels 0-9)
// Each channel uses 3 bits for sample time selection
#define ADC_SMPR2_SMP0        (0x7U << 0)   // Channel 0 sample time
#define ADC_SMPR2_SMP1        (0x7U << 3)   // Channel 1 sample time (PA1 = ADC1_CH1)
#define ADC_SMPR2_SMP2        (0x7U << 6)   // Channel 2 sample time
// ... (add more as needed)

// Sample time values (cycles)
#define ADC_SMP_3CYCLES       0x0U
#define ADC_SMP_15CYCLES      0x1U
#define ADC_SMP_28CYCLES      0x2U
#define ADC_SMP_56CYCLES      0x3U
#define ADC_SMP_84CYCLES      0x4U
#define ADC_SMP_112CYCLES     0x5U
#define ADC_SMP_144CYCLES     0x6U
#define ADC_SMP_480CYCLES     0x7U

// RCC_APB2ENR Bit for ADC1
#define RCC_APB2ENR_ADC1EN    (1U << 8)   // Bit 8: Enable ADC1 clock

/* ========================================================================
 * TIM3 - General Purpose Timer (APB1 Bus)
 * ======================================================================== */
#define TIM3_BASE             ((uint32_t)0x40000400U)

#define TIM3_CR1              (*(volatile uint32_t*)(TIM3_BASE + 0x00U))
#define TIM3_EGR              (*(volatile uint32_t*)(TIM3_BASE + 0x14U))
#define TIM3_CCMR1            (*(volatile uint32_t*)(TIM3_BASE + 0x18U))
#define TIM3_CCER             (*(volatile uint32_t*)(TIM3_BASE + 0x20U))
#define TIM3_PSC              (*(volatile uint32_t*)(TIM3_BASE + 0x28U))
#define TIM3_ARR              (*(volatile uint32_t*)(TIM3_BASE + 0x2CU))
#define TIM3_CCR1             (*(volatile uint32_t*)(TIM3_BASE + 0x34U))

/* ========================================================================
 * I2C1 - Inter-Integrated Circuit (APB1 Bus)
 * ======================================================================== */
#define I2C1_BASE             ((uint32_t)0x40005400U)

#define I2C1_CR1              (*(volatile uint32_t*)(I2C1_BASE + 0x00U))
#define I2C1_CR2              (*(volatile uint32_t*)(I2C1_BASE + 0x04U))
#define I2C1_OAR1             (*(volatile uint32_t*)(I2C1_BASE + 0x08U))
#define I2C1_DR               (*(volatile uint32_t*)(I2C1_BASE + 0x10U))
#define I2C1_SR1              (*(volatile uint32_t*)(I2C1_BASE + 0x14U))
#define I2C1_SR2              (*(volatile uint32_t*)(I2C1_BASE + 0x18U))
#define I2C1_CCR              (*(volatile uint32_t*)(I2C1_BASE + 0x1CU))
#define I2C1_TRISE            (*(volatile uint32_t*)(I2C1_BASE + 0x20U))

// I2C1 Control Bits
#define I2C_CR1_PE            (1U << 0)   // Peripheral Enable
#define I2C_CR1_START         (1U << 8)   // Start Generation
#define I2C_CR1_STOP          (1U << 9)   // Stop Generation
#define I2C_CR1_ACK           (1U << 10)  // Acknowledge Enable

// I2C1 Status Bits
#define I2C_SR1_SB            (1U << 0)   // Start Bit (Master mode)
#define I2C_SR1_ADDR          (1U << 1)   // Address sent (master) / matched (slave)
#define I2C_SR1_TXE           (1U << 7)   // Data register empty (transmitters)
#define I2C1_SR1_ACK_FAILURE  (1U << 10)  // Acknowledge Failure Flag: ( 1- ACK Failed, 0- ACK Not failed)

// RCC APB1ENR Bit for I2C1
#define RCC_APB1ENR_I2C1EN    (1U << 21)  // Bit 21: Enable I2C1 clock

// RCC APB1ENR Bit for TIM3
#define RCC_APB1ENR_TIM3EN    (1U << 1)   // Bit 1: Enable TIM3 clock

#endif /* BLACKPILL_STM32F411_H */
