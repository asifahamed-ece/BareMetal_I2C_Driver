#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

void timebase_init();

uint32_t timebase_us(void);

void delay_us(uint32_t us);

#endif