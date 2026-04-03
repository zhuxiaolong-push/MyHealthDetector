#ifndef BUZZER_H
#define BUZZER_H

#include "stm32f4xx.h"
#include "Com_Delay.h"

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint32_t ms);

#endif
