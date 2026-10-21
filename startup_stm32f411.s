/* ===========================================================================
 * startup_stm32f411.s — Reset_Handler and vector table for STM32F411CEU6
 * ===========================================================================
 *
 * Part of the baremetal I2C driver learning project.
 * Pairs with stm32f411.ld — that file defines the addresses, this one uses them.
 *
 * WHAT THE CPU DOES ON RESET (RM0383 §2.2.3, PM0214 §2.3.4):
 *   1. Reads the initial SP from 0x00000000
 *   2. Reads the reset vector from 0x00000004
 *   3. Both come from .isr_vector, which must be the FIRST bytes in flash
 *      because 0x00000000 is an alias of 0x08000000.
 *
 * VECTOR TABLE SIZE: 102 words = 408 bytes = 0x198
 *   16 system exceptions + 86 external IRQ slots (positions 0..85)
 *   29 of the 86 slots are reserved on the F411 but still occupy address
 *   space, so they get Default_Handler rather than 0.
 *
 * IRQ NAMES: taken from RM0383 Table 37 via docs/reference/f411_external_irqs.inc
 *
 * VERIFIED (on this machine, GCC 16.2.0 / ld 2.47):
 *   - .isr_vector is 0x198 bytes = 102 words
 *   - .isr_vector has the "A" (alloc) flag and sits at 0x08000000
 *   - .text starts at 0x08000198, so the two do not overlap
 *   - the reset sequence was EXECUTED on a Cortex-M4 simulator (Unicorn):
 *       FPU enabled (CPACR = 0x00F00000), .data copied (value 7 correct),
 *       .bss zeroed, main() reached with SP = 0x20020000
 *   - IRQ vector addressing checked: IRQ 85 -> 0x194 = 0x40 + 4*85
 *   - Default_Handler verified to loop forever
 *
 * NOT YET VERIFIED ON HARDWARE: the ST-Link was not connected when this was
 * written, so the blinky has not been flashed and observed yet. That is the
 * next step.
 * =========================================================================== */

  .syntax unified
  .cpu cortex-m4
  .fpu fpv4-sp-d16
  .thumb

/* ---------------------------------------------------------------------------
 * Declare a handler as a weak alias of Default_Handler.
 *
 * Weak means: if the application defines a strong symbol with this name, the
 * application's version wins automatically. If it never does, the alias
 * supplies Default_Handler so the vector table never points at nothing.
 * ---------------------------------------------------------------------- */
  .macro def_handler name
    .weak \name
    .thumb_set \name, Default_Handler
  .endm

/* ===========================================================================
 * VECTOR TABLE
 *
 * The "a" in .section .isr_vector,"a",%progbits means ALLOCATABLE.
 * Without it the section is not loadable: the linker does not advance FLASH
 * for it, .text lands on top of it at 0x08000000, and .isr_vector ends up in
 * no program header so it is never flashed at all. The build succeeds and the
 * chip locks up. Check with:
 *   arm-none-eabi-readelf -S build/app.elf | grep isr_vector   # needs flag A
 * ========================================================================= */
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:

  .word _estack              /* 0  : initial stack pointer (0x20020000) */
  .word Reset_Handler        /* 1  : reset                     */
  .word NMI_Handler          /* 2  */
  .word HardFault_Handler    /* 3  */
  .word MemManage_Handler    /* 4  */
  .word BusFault_Handler     /* 5  */
  .word UsageFault_Handler   /* 6  */
  .word 0                    /* 7  : reserved */
  .word 0                    /* 8  : reserved */
  .word 0                    /* 9  : reserved */
  .word 0                    /* 10 : reserved */
  .word SVC_Handler          /* 11 */
  .word DebugMon_Handler     /* 12 */
  .word 0                    /* 13 : reserved */
  .word PendSV_Handler       /* 14 */
  .word SysTick_Handler      /* 15 */

  /* ---- external IRQs, positions 0..85 (86 slots) ---- */
  .word WWDG_IRQHandler
  .word PVD_IRQHandler
  .word TAMP_STAMP_IRQHandler
  .word RTC_WKUP_IRQHandler
  .word FLASH_IRQHandler
  .word RCC_IRQHandler
  .word EXTI0_IRQHandler
  .word EXTI1_IRQHandler
  .word EXTI2_IRQHandler
  .word EXTI3_IRQHandler
  .word EXTI4_IRQHandler
  .word DMA1_Stream0_IRQHandler
  .word DMA1_Stream1_IRQHandler
  .word DMA1_Stream2_IRQHandler
  .word DMA1_Stream3_IRQHandler
  .word DMA1_Stream4_IRQHandler
  .word DMA1_Stream5_IRQHandler
  .word DMA1_Stream6_IRQHandler
  .word ADC_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word EXTI9_5_IRQHandler
  .word TIM1_BRK_TIM9_IRQHandler
  .word TIM1_UP_TIM10_IRQHandler
  .word TIM1_TRG_COM_TIM11_IRQHandler
  .word TIM1_CC_IRQHandler
  .word TIM2_IRQHandler
  .word TIM3_IRQHandler
  .word TIM4_IRQHandler
  .word I2C1_EV_IRQHandler
  .word I2C1_ER_IRQHandler
  .word I2C2_EV_IRQHandler
  .word I2C2_ER_IRQHandler
  .word SPI1_IRQHandler
  .word SPI2_IRQHandler
  .word USART1_IRQHandler
  .word USART2_IRQHandler
  .word Default_Handler
  .word EXTI15_10_IRQHandler
  .word EXTI17_IRQHandler
  .word EXTI18_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word DMA1_Stream7_IRQHandler
  .word Default_Handler
  .word SDIO_IRQHandler
  .word TIM5_IRQHandler
  .word SPI3_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word DMA2_Stream0_IRQHandler
  .word DMA2_Stream1_IRQHandler
  .word DMA2_Stream2_IRQHandler
  .word DMA2_Stream3_IRQHandler
  .word DMA2_Stream4_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word OTG_FS_IRQHandler
  .word DMA2_Stream5_IRQHandler
  .word DMA2_Stream6_IRQHandler
  .word DMA2_Stream7_IRQHandler
  .word USART6_IRQHandler
  .word I2C3_EV_IRQHandler
  .word I2C3_ER_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word FPU_IRQHandler
  .word Default_Handler
  .word Default_Handler
  .word SPI4_IRQHandler
  .word SPI5_IRQHandler

/* ===========================================================================
 * DEFAULT HANDLER
 *
 * Every unused vector lands here. The CPU parks in an infinite loop so a
 * debugger can attach and inspect the state that caused the fault.
 * ========================================================================= */
  .section .text.Default_Handler,"ax",%progbits
  .type Default_Handler, %function
  .weak Default_Handler
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/* ===========================================================================
 * RESET HANDLER
 *
 * Order is deliberate:
 *   1. Enable the FPU. It is disabled from reset (PM0214 §4.6.1). The build
 *      uses -mfloat-abi=hard, so any float instruction before this would
 *      raise a NOCP UsageFault.
 *   2. Copy .data from flash to RAM. These variables have non-zero
 *      initialisers, so the initialiser bytes live in flash but the variable
 *      must live in RAM.
 *   3. Zero .bss. These have no initialiser, they just need RAM.
 *   4. Call main.
 * ========================================================================= */
  .section .text.Reset_Handler,"ax",%progbits
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:

/* --- 1. Enable the FPU (CPACR, PM0214 §4.6.6) ---
 * CPACR is at 0xE000ED88. Bits 23:20 hold CP11/CP10 access; 0b11 in each
 * field = full access. DSB then ISB so the pipeline sees the change. */
  ldr r0, =0xE000ED88
  ldr r1, [r0]
  orr r1, r1, #(0xF << 20)
  str r1, [r0]
  dsb
  isb

/* --- 2. Copy .data: flash -> RAM ---
 * src = _sidata (LMA, in flash)   dst = _sdata.._edata (VMA, in RAM) */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
CopyDataInit:
  cmp r0, r1
  bcs ZeroBssInit
  ldr r3, [r2], #4
  str r3, [r0], #4
  b CopyDataInit

/* --- 3. Zero .bss: _sbss.._ebss --- */
ZeroBssInit:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
FillZerobss:
  cmp r0, r1
  bcs CallMain
  str r2, [r0], #4
  b FillZerobss

/* --- 4. Call main --- */
CallMain:
  bl main
InfiniteLoop:
  b InfiniteLoop
  .size Reset_Handler, .-Reset_Handler

/* ===========================================================================
 * WEAK ALIASES
 *
 * Every handler name used in the vector table must exist. These make each one
 * resolve to Default_Handler unless the application provides its own.
 *
 * NOTE on __libc_init_array: NOT called. This project is plain C11 with no
 * C++ and no __attribute__((constructor)) functions, so there are no static
 * constructors to run. The F103 reference calls it; we deliberately do not.
 * ========================================================================= */
  def_handler NMI_Handler
  def_handler HardFault_Handler
  def_handler MemManage_Handler
  def_handler BusFault_Handler
  def_handler UsageFault_Handler
  def_handler SVC_Handler
  def_handler DebugMon_Handler
  def_handler PendSV_Handler
  def_handler SysTick_Handler

  def_handler WWDG_IRQHandler
  def_handler PVD_IRQHandler
  def_handler TAMP_STAMP_IRQHandler
  def_handler RTC_WKUP_IRQHandler
  def_handler FLASH_IRQHandler
  def_handler RCC_IRQHandler
  def_handler EXTI0_IRQHandler
  def_handler EXTI1_IRQHandler
  def_handler EXTI2_IRQHandler
  def_handler EXTI3_IRQHandler
  def_handler EXTI4_IRQHandler
  def_handler DMA1_Stream0_IRQHandler
  def_handler DMA1_Stream1_IRQHandler
  def_handler DMA1_Stream2_IRQHandler
  def_handler DMA1_Stream3_IRQHandler
  def_handler DMA1_Stream4_IRQHandler
  def_handler DMA1_Stream5_IRQHandler
  def_handler DMA1_Stream6_IRQHandler
  def_handler ADC_IRQHandler
  def_handler EXTI9_5_IRQHandler
  def_handler TIM1_BRK_TIM9_IRQHandler
  def_handler TIM1_UP_TIM10_IRQHandler
  def_handler TIM1_TRG_COM_TIM11_IRQHandler
  def_handler TIM1_CC_IRQHandler
  def_handler TIM2_IRQHandler
  def_handler TIM3_IRQHandler
  def_handler TIM4_IRQHandler
  def_handler I2C1_EV_IRQHandler
  def_handler I2C1_ER_IRQHandler
  def_handler I2C2_EV_IRQHandler
  def_handler I2C2_ER_IRQHandler
  def_handler SPI1_IRQHandler
  def_handler SPI2_IRQHandler
  def_handler USART1_IRQHandler
  def_handler USART2_IRQHandler
  def_handler EXTI15_10_IRQHandler
  def_handler EXTI17_IRQHandler
  def_handler EXTI18_IRQHandler
  def_handler DMA1_Stream7_IRQHandler
  def_handler SDIO_IRQHandler
  def_handler TIM5_IRQHandler
  def_handler SPI3_IRQHandler
  def_handler DMA2_Stream0_IRQHandler
  def_handler DMA2_Stream1_IRQHandler
  def_handler DMA2_Stream2_IRQHandler
  def_handler DMA2_Stream3_IRQHandler
  def_handler DMA2_Stream4_IRQHandler
  def_handler OTG_FS_IRQHandler
  def_handler DMA2_Stream5_IRQHandler
  def_handler DMA2_Stream6_IRQHandler
  def_handler DMA2_Stream7_IRQHandler
  def_handler USART6_IRQHandler
  def_handler I2C3_EV_IRQHandler
  def_handler I2C3_ER_IRQHandler
  def_handler FPU_IRQHandler
  def_handler SPI4_IRQHandler
  def_handler SPI5_IRQHandler
