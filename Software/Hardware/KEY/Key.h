#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f4xx.h"

// 页面定义
typedef enum {
    KEY_PAGE_NONE = 0,
    KEY_PAGE_HOMEPAGE,
    KEY_PAGE_HEARTRATE,
    KEY_PAGE_TEMPERATURE,
    KEY_PAGE_SETTINGS,
    KEY_PAGE_ABOUT,
    KEY_PAGE_POWER
} Key_Page_t;

// 功能按键硬件定义 - KEY1: PA15, KEY2: PC6, KEY3: PC7
#define KEY1_PIN        	GPIO_Pin_15
#define KEY1_PORT   		GPIOA
#define KEY1_CLK    		RCC_AHB1Periph_GPIOA

#define KEY2_PIN        	GPIO_Pin_6  
#define KEY2_PORT   		GPIOC
#define KEY2_CLK    		RCC_AHB1Periph_GPIOC

#define KEY3_PIN        	GPIO_Pin_7
#define KEY3_PORT   		GPIOC
#define KEY3_CLK    		RCC_AHB1Periph_GPIOC

// 电源控制引脚定义
#define POWER_CTRL_PORT     GPIOD
#define POWER_CTRL_PIN      GPIO_Pin_15
#define POWER_KEY_PORT      GPIOD
#define POWER_KEY_PIN       GPIO_Pin_14
#define POWER_KEY_EXTI_LINE EXTI_Line14
#define POWER_KEY_IRQn      EXTI15_10_IRQn

// 信号量外部声明
extern SemaphoreHandle_t key_semaphore;

// 任务函数声明
void Key_Detect_Task(void *pvParameters);

// 页面控制接口
void Key_SetActivePage(uint8_t page);
void Key_ResetState(void);
Key_Page_t Key_GetCurrentPage(void);

// 电源状态重置
void Key_ResetPowerState(void);

// 测量控制
void Handle_Key3_Measurement(void);

// 硬件初始化
void Key_Init(void);

#endif // __KEY_H