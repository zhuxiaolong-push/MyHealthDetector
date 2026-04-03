#ifndef __BACKLIGHT_H__
#define __BACKLIGHT_H__

#include "stm32f4xx.h"

#define BACKLIGHT_GPIO_PORT     GPIOB
#define BACKLIGHT_GPIO_PIN      GPIO_Pin_0
#define BACKLIGHT_GPIO_CLK      RCC_AHB1Periph_GPIOB
#define BACKLIGHT_TIM_CLK       RCC_APB1Periph_TIM3

#define PWM_PERIOD              3359            // 25kHz
#define BACKLIGHT_MAX_VALUE     100
#define BACKLIGHT_MIN_VALUE     5

void Backlight_Init(void);
void Backlight_SetBrightness(uint8_t percent);
uint8_t Backlight_GetBrightness(void);
void Backlight_On(void);
void Backlight_Off(void);

// 新增：快捷设置0%和100%
void Backlight_SetMin(void);    // 0%（关闭）
void Backlight_SetMax(void);    // 100%（最亮）

#endif