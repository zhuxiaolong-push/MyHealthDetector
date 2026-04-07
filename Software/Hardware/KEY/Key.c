#include "key.h"

// 按键配置表
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    KeyNum_t id;
    BitAction active;
} KeyConfig_t;

static const KeyConfig_t measure_keys[] = {
    {GPIOA, GPIO_Pin_15, KEY1, Bit_RESET},
    {GPIOC, GPIO_Pin_6,  KEY2, Bit_RESET},
    {GPIOC, GPIO_Pin_7,  KEY3, Bit_RESET}
};
#define MEASURE_KEY_COUNT (sizeof(measure_keys) / sizeof(measure_keys[0]))

// 初始化
void Key_Init(void) {
    // 创建信号量（由 App_task.c 创建更好，但这里保持兼容）
    if (key_semaphore == NULL) {
        key_semaphore = xSemaphoreCreateBinary();
    }

    // 使能 GPIO 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    // 配置测量按键 PA15, PC6, PC7
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Mode = GPIO_Mode_IN;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;

    gpio_init.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOC, &gpio_init);
    
    // 电源键配置（上拉输入）
    gpio_init.GPIO_Pin = POWER_KEY_PIN;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(POWER_KEY_PORT, &gpio_init);
    
    // 电源控制引脚配置（推挽输出，默认高电平保持开机）
    gpio_init.GPIO_Pin = POWER_CTRL_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(POWER_CTRL_PORT, &gpio_init);
    GPIO_SetBits(POWER_CTRL_PORT, POWER_CTRL_PIN);
}

// 按键扫描（单次扫描，用于任务轮询）
uint8_t Key_IsPressed(KeyNum_t key) {
    for (uint8_t i = 0; i < MEASURE_KEY_COUNT; i++) {
        if (measure_keys[i].id == key) {
            return GPIO_ReadInputDataBit(measure_keys[i].port, measure_keys[i].pin) == measure_keys[i].active;
        }
    }
    return 0;
}

// 电源键读取
uint8_t PowerKey_IsPressed(void) {
    return GPIO_ReadInputDataBit(POWER_KEY_PORT, POWER_KEY_PIN) == Bit_RESET;
}

// 扫描所有按键，返回第一个按下的键（带简单消抖）
KeyNum_t Key_Scan(void) {
    for (uint8_t i = 0; i < MEASURE_KEY_COUNT; i++) {
        if (GPIO_ReadInputDataBit(measure_keys[i].port, measure_keys[i].pin) == measure_keys[i].active) {
            return measure_keys[i].id;
        }
    }
    return KEY_NONE;
}

// 电源键中断服务程序（必须在驱动层，因为涉及硬件中断）
void EXTI15_10_IRQHandler(void) {
    if (EXTI_GetITStatus(POWER_KEY_EXTI_LINE) != RESET) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (key_semaphore != NULL) {
            xSemaphoreGiveFromISR(key_semaphore, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        EXTI_ClearITPendingBit(POWER_KEY_EXTI_LINE);
    }
}
