#include "backlight.h"

static uint8_t current_brightness = 80;

void Backlight_Init(void)
{
    RCC_AHB1PeriphClockCmd(BACKLIGHT_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(BACKLIGHT_TIM_CLK, ENABLE);
    
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = BACKLIGHT_GPIO_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(BACKLIGHT_GPIO_PORT, &gpio_init);
    
    GPIO_PinAFConfig(BACKLIGHT_GPIO_PORT, GPIO_PinSource0, GPIO_AF_TIM3);
    
    TIM_TimeBaseInitTypeDef tim_init;
    TIM_TimeBaseStructInit(&tim_init);
    tim_init.TIM_Prescaler = 0;
    tim_init.TIM_Period = PWM_PERIOD;
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tim_init);
    
    TIM_OCInitTypeDef oc_init;
    TIM_OCStructInit(&oc_init);
    oc_init.TIM_OCMode = TIM_OCMode_PWM1;
    oc_init.TIM_OutputState = TIM_OutputState_Enable;
    oc_init.TIM_Pulse = 2720;  // 80% * 3360 ≈ 2720
    oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC3Init(TIM3, &oc_init);
    
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
    TIM_ITConfig(TIM3, TIM_IT_CC3, DISABLE);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
    
    current_brightness = 100;
}

void Backlight_SetBrightness(uint8_t percent)
{
    if (percent > BACKLIGHT_MAX_VALUE) percent = BACKLIGHT_MAX_VALUE;
    if (percent > 0 && percent < BACKLIGHT_MIN_VALUE) percent = BACKLIGHT_MIN_VALUE;
    
    current_brightness = percent;
    TIM3->CCR3 = percent * 34;  // 3360/100 ≈ 34
}

// 新增：设置0%（关闭）
void Backlight_SetMin(void)
{
    current_brightness = 0;
    TIM3->CCR3 = 0;
}

// 新增：设置100%（最亮）
void Backlight_SetMax(void)
{
    current_brightness = 100;
    TIM3->CCR3 = 3360;  // 100% = 3360
}

uint8_t Backlight_GetBrightness(void)
{
    return current_brightness;
}

void Backlight_On(void)
{
    Backlight_SetBrightness(current_brightness > 0 ? current_brightness : 100);
}

void Backlight_Off(void)
{
    TIM3->CCR3 = 0;
    current_brightness = 0;
}