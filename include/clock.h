#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void clock_init_100Mhz(void);
uint32_t clock_get_pclk1_hz(void);

#endif