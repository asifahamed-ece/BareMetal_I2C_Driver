# include "clock.h"
# include "blackpill_stm32f411.h"

#define PLLM 16U
#define PLLN 200U
#define PLLP 0U // PLLP = 2 :> 0b00
#define PLLQ 4U

uint32_t clock_get_pclk1_hz(void) {
    return 50000000;
}

void clock_init_100Mhz()
{   
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;   // Enable PWR Clock
    
    PWR_CR &= ~(VOS_Scale1);
    PWR_CR |= VOS_Scale1;  // Set the Voltage Scaling to 1

    FLASH_ACR |= FLASH_LATENCY_3WAITS;    // Flash latency Selection : 3 wait states for 100MHz
    FLASH_ACR |= (FLASH_PRFTEN | FLASH_ICEN | FLASH_DCEN);

    RCC_PLLCFGR = ( (PLLM  << RCC_PLLCFGR_PLLM_Pos)  | 
                    (PLLN << RCC_PLLCFGR_PLLN_Pos)   | 
                    (PLLP  << RCC_PLLCFGR_PLLP_Pos)  | 
                    (PLLQ  << RCC_PLLCFGR_PLLQ_Pos)  |
                    (RCC_PLLSRC_HSI)  );
    
    // Enable PLL
    RCC_CR |= RCC_CR_PLL_ON;
    
    // Spin Until PLL_RDY bit is Enabled
    while( !(RCC_CR & RCC_CR_PLL_RDY));

    // Configure RCC CLock Prescalers
    RCC_CFGR &= ~( (0xFU << 4U ) |
                   (0x7U << 10U) |
                   (0x7U << 13U) |
                   (0x3U << 0U ) );  
    RCC_CFGR |= ( (0x0U << 4U ) |
                  (0x4U << 10U) |
                  (0x0U << 13U) |
                  (0x2U << 0U ) );

    // Spinning Until System Clock Status is Set as per Configs
    while((((RCC_CFGR >> 2U) & 0x3U) != 0x2U));   // Extract last 2 bits and Compare
    // Waits Until the HSI is Selected (btw its the Default)

}