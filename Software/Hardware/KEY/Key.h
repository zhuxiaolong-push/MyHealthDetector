#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "Com_Delay.h"

// FreeRTOS 头文件必须按顺序包含
#include "FreeRTOS.h"
#include "semphr.h"

// ========== 按键编号定义 ==========
typedef enum {
    KEY_NONE = 0,
    KEY1,       // PA15 - 进入心率页面
    KEY2,       // PC6  - 进入体温页面  
    KEY3        // PC7  - 开始/停止测量
} KeyNum_t;

// ========== 页面类型定义 ==========
#define KEY_PAGE_NONE           0
#define KEY_PAGE_HEART_RATE     1
#define KEY_PAGE_TEMPERATURE    2

// ========== 电源键定义 ==========
#define POWER_CTRL_PIN          GPIO_Pin_15
#define POWER_CTRL_PORT         GPIOD
#define POWER_KEY_PIN           GPIO_Pin_14
#define POWER_KEY_PORT          GPIOD
#define POWER_KEY_IRQn          EXTI15_10_IRQn
#define POWER_KEY_EXTI_LINE     EXTI_Line14

// ========== 时间参数定义 ==========
#define KEY_DEBOUNCE_MS         30
#define KEY_LONG_PRESS_MS       2000    // 长按2秒触发关机界面

// ========== 电源键状态机 ==========
typedef enum {
    KEY_STATE_IDLE,             // 空闲状态
    KEY_STATE_DEBOUNCE,         // 消抖状态
    KEY_STATE_WAIT_RELEASE,     // 等待释放（同时计时长按）
    KEY_STATE_LONG_PRESS_DONE   // 长按已触发（结束态）
} KeyState_t;

// ========== 函数声明 ==========

// 初始化和任务
void Key_Init(void);
void Key_Detect_Task(void *pvParameters);

// 兼容层（保留原函数）
KeyNum_t Key_GetNum(void);
KeyNum_t Key_Scan(void);

// 状态管理
void Key_ResetState(void);
void Key_SetActivePage(uint8_t page_type);
void Key_ResetPowerState(void);     // 供关机界面调用

#endif