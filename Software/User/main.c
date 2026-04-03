/*
 * @Author: 朱晓龙
 * @Date: 2026-03-16 23:55:18
 * @LastEditTime: 2026-03-24 11:36:33
 * @LastEditors:朱晓龙
 * @Description: 
 * @FilePath: \Software\User\main.c
 * 可以输入预定的版权声明、个性签名、空行等
 */
#include "stm32f4xx.h"
#include "Com_Debug.h"
#include "LCD.h"
#include "App_task.h"
#include "GUI_main.h"
#include "Com_Delay.h"
#include "touch.h"
#include "GUI_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "W25Q16.h"
#include "Key.h"


#define LVGL_LOG_ENABLE 1

#if LVGL_LOG_ENABLE == 1
void my_log_cb(const char *buf) {
    printf("%s", buf);   //LVGL日志重定向
}
#endif

void Power_Start(void);

int main(void)
{	
	SystemInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SysTick_Config(SystemCoreClock / 1000);
	delay_init();
    W25Q16_Init();
    Com_Debug_Init();
	Power_Start(); 
	lv_init();	
	#if LVGL_LOG_ENABLE == 1
	// 注册日志回调
    lv_log_register_print_cb(my_log_cb);
	#endif
	lv_port_disp_init();
	lv_port_indev_init();
	App_task_init();
	while(1)
	{
		
	}
}

// 按键按下后，拉高 PD15，程序开机
void Power_Start(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 电源控制引脚初始化
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = POWER_CTRL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(POWER_CTRL_PORT, &GPIO_InitStructure);
    
    // 默认拉高 (开机状态)
    GPIO_SetBits(POWER_CTRL_PORT, POWER_CTRL_PIN);

    //电源检测按键初始化
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = POWER_KEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(POWER_CTRL_PORT, &GPIO_InitStructure);

    //配置外部中断
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource14);

    EXTI_InitStructure.EXTI_Line = POWER_KEY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // 下降沿触发 (按下)
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    //配置 NVIC
    NVIC_InitStructure.NVIC_IRQChannel = POWER_KEY_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

