#include "buzzer.h"

/* 定义蜂鸣器引脚：PA3 */
#define BUZZER_PORT   GPIOA
#define BUZZER_PIN    GPIO_Pin_3

/* 简单延时函数（使用循环，非精确） */
static void buzzer_delay_ms(uint32_t ms)
{
	delay_ms(ms);
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能 GPIOA 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    
    /* 配置 PA3 为推挽输出，速度 50MHz */
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
    
    /* 初始状态为关闭 */
    Buzzer_Off();
}

void Buzzer_On(void)
{
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Off(void)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Beep(uint32_t ms)
{
    Buzzer_On();
    buzzer_delay_ms(ms);
    Buzzer_Off();
}