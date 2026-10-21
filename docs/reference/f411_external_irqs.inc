/* ======================================================================
 * F411CEU6 EXTERNAL IRQ SLOT LIST - extracted from RM0383 Table 37
 *
 * 86 slots, positions 0..85.  57 named, 29 reserved on STM32F411xC/E.
 * Address of slot n = 0x40 + 4*n.  Last slot (SPI5, pos 85) = 0x194.
 *
 * These are the NAMES to use in your startup_stm32f411.s.
 * Note: RM0383 uses "EXTI17 / RTC_Alarm" and "EXTI18 / OTG_FS_WKUP" for
 * positions 41 and 42 - two sources share one vector. Pick one name, or
 * name it for the peripheral you actually use.
 *
 * Positions 1, 2, 3 are EXTI16, EXTI21, EXTI22 in the F411 (they are PVD,
 * TAMPER and RTC_WKUP on the F103) - the numbers differ between families.
 *
 * Reserved slots still need a value: Default_Handler, not 0.
 * ====================================================================== */

  .word WWDG_IRQHandler
  .word PVD_IRQHandler             /* EXTI16 */
  .word TAMP_STAMP_IRQHandler     /* EXTI21 */
  .word RTC_WKUP_IRQHandler       /* EXTI22 */
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
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
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
  .word Default_Handler        /* reserved */
  .word EXTI15_10_IRQHandler
  .word EXTI17_IRQHandler
  .word EXTI18_IRQHandler
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word DMA1_Stream7_IRQHandler
  .word Default_Handler        /* reserved */
  .word SDIO_IRQHandler
  .word TIM5_IRQHandler
  .word SPI3_IRQHandler
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word DMA2_Stream0_IRQHandler
  .word DMA2_Stream1_IRQHandler
  .word DMA2_Stream2_IRQHandler
  .word DMA2_Stream3_IRQHandler
  .word DMA2_Stream4_IRQHandler
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word OTG_FS_IRQHandler
  .word DMA2_Stream5_IRQHandler
  .word DMA2_Stream6_IRQHandler
  .word DMA2_Stream7_IRQHandler
  .word USART6_IRQHandler
  .word I2C3_EV_IRQHandler
  .word I2C3_ER_IRQHandler
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word FPU_IRQHandler
  .word Default_Handler        /* reserved */
  .word Default_Handler        /* reserved */
  .word SPI4_IRQHandler
  .word SPI5_IRQHandler
