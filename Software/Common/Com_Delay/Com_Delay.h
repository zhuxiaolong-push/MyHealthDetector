#ifndef COM_DELAY_H
#define COM_DELAY_H

#include "stm32f4xx.h"
#include <stdint.h>

void delay_init(void);
void delay_us(uint32_t _us);
void delay_ms(uint32_t _ms);
void delay_1ms(uint32_t ms);
void delay_1us(uint32_t us);

#endif // COM_DELAY_H
