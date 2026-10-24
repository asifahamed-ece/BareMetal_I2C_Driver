/**
 * @file    clock.c
 * @brief   HSI -> PLL -> 100 MHz clock tree for the STM32F411CEU6.
 */

# include "clock.h"
# include "blackpill_stm32f411.h"

/*
 * PLL chain (RM0383 §6.3.2):
 *   HSI 16 MHz / PLLM 16  = 1 MHz VCO input    (allowed: 1-2 MHz)
 *   VCO in x PLLN 200     = 200 MHz VCO out    (allowed: 100-432 MHz)
 *   VCO out / PLLP        = 100 MHz SYSCLK     (PLLP = 0b00 means P = 2)
 *   VCO out / PLLQ 4      = 50 MHz             (48 MHz domain, unused here)
 */
#define PLLM 16U
#define PLLN 200U
#define PLLP 0U   ///< 0b00 => P = 2
#define PLLQ 4U

uint32_t clock_get_pclk1_hz(void) {
    return 50000000;
}

/**
 * @brief Switch SYSCLK from HSI 16 MHz to the PLL at 100 MHz.
 *
 * Order matters. Regulator voltage and flash wait states belong to the
 * *faster* clock, so both are raised while the core is still slow. Only then
 * is SW moved to the PLL.
 *
 * @warning Blocking, with no timeout on either spin loop.
 *
 * @warning No settle delay after the VOS change. RM0383 §5.1.4 requires the
 *          regulator to be ready before SYSCLK is raised.
 *
 * @note Writes RCC_CFGR in one go: AHB /1, APB1 /2, APB2 /1, SW = PLL.
 */
void clock_init_100Mhz()
{   
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;   /* PWR registers are dead without this */
    
    PWR_CR &= ~(VOS_Scale1);
    PWR_CR |= VOS_Scale1;  /* VOS = 0b11, scale 1 for SYSCLK <= 100 MHz */

    FLASH_ACR |= FLASH_LATENCY_3WAITS;    /* 3 wait states at 100 MHz, 2.7-3.6 V */
    FLASH_ACR |= (FLASH_PRFTEN | FLASH_ICEN | FLASH_DCEN);

    /* Whole-register write. PLLSRC = 0 keeps HSI as the PLL input. */
    RCC_PLLCFGR = ( (PLLM  << RCC_PLLCFGR_PLLM_Pos)  | 
                    (PLLN << RCC_PLLCFGR_PLLN_Pos)   | 
                    (PLLP  << RCC_PLLCFGR_PLLP_Pos)  | 
                    (PLLQ  << RCC_PLLCFGR_PLLQ_Pos)  |
                    (RCC_PLLSRC_HSI)  );
    
    /* PLL on, then wait for lock. */
    RCC_CR |= RCC_CR_PLL_ON;
    
    while( !(RCC_CR & RCC_CR_PLL_RDY));

    /* Prescalers: HPRE /1, PPRE1 /2, PPRE2 /1, SW = PLL. */
    RCC_CFGR &= ~( (0xFU << 4U ) |
                   (0x7U << 10U) |
                   (0x7U << 13U) |
                   (0x3U << 0U ) );  
    RCC_CFGR |= ( (0x0U << 4U ) |
                  (0x4U << 10U) |
                  (0x0U << 13U) |
                  (0x2U << 0U ) );

    /* SWS (bits 3:2) reads 0b10 once the PLL is actually driving SYSCLK. */
    while((((RCC_CFGR >> 2U) & 0x3U) != 0x2U));

}
