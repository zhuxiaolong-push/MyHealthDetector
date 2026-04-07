#include "key.h"
#include "GUI_HeartRate.h"
#include "GUI_Temperature.h"
#include "GUI_Power.h"
#include "task.h"

// ========== 按键配置 ==========
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    KeyNum_t id;
    BitAction active;
} KeyConfig_t;

// 测量按键配置表
static const KeyConfig_t measure_keys[] = {
    {GPIOA, GPIO_Pin_15, KEY1, Bit_RESET},  // 心率页面
    {GPIOC, GPIO_Pin_6,  KEY2, Bit_RESET},  // 体温页面
    {GPIOC, GPIO_Pin_7,  KEY3, Bit_RESET}   // 开始/停止测量
};
#define MEASURE_KEY_COUNT (sizeof(measure_keys) / sizeof(measure_keys[0]))

// 当前激活的测量页面类型
typedef enum {
    PAGE_NONE = 0,
    PAGE_HEART_RATE,
    PAGE_TEMPERATURE
} ActivePage_t;

static volatile ActivePage_t active_page = PAGE_NONE;
static volatile uint8_t key3_measure_state = 0;  // 0=停止, 1=测量中

// 电源键状态机
static KeyState_t power_key_state = KEY_STATE_IDLE;
static TickType_t power_press_start_time = 0;
static bool shutdown_ui_shown = false;

// 按键信号量（用于电源键中断）
static SemaphoreHandle_t key_semaphore = NULL;

// ========== 底层函数 ==========
static uint8_t Key_IsPressed(const KeyConfig_t* key) {
    return GPIO_ReadInputDataBit(key->port, key->pin) == key->active;
}

static uint8_t PowerKey_IsPressed(void) {
    return GPIO_ReadInputDataBit(POWER_KEY_PORT, POWER_KEY_PIN) == Bit_RESET;
}

__weak void Key_DelayMs(uint32_t ms) {
    delay_ms(ms);
}

// ========== 初始化 ==========
void Key_Init(void) {
    // 创建信号量
    key_semaphore = xSemaphoreCreateBinary();

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
}

// 电源键中断服务程序
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(POWER_KEY_EXTI_LINE) != RESET) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(key_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        EXTI_ClearITPendingBit(POWER_KEY_EXTI_LINE);
    }
}

// ========== 按键处理函数 ==========
static void Handle_Key1(void) {
    LV_LOG_USER("KEY1 pressed - Navigate to Heart Rate page");
    active_page = PAGE_HEART_RATE;
    key3_measure_state = 0;
    GUI_Load_HeartRatePage();
}

static void Handle_Key2(void) {
    LV_LOG_USER("KEY2 pressed - Navigate to Temperature page");
    active_page = PAGE_TEMPERATURE;
    key3_measure_state = 0;
    GUI_Load_TemperaturePage();
}

static void Handle_Key3(void) {
    LV_LOG_USER("KEY3 pressed - Toggle measurement");

    if (active_page == PAGE_HEART_RATE) {
        if (key3_measure_state == 0) {
            LV_LOG_USER("Start Heart Rate measurement");
            HeartRate_StartMeasurement();
            key3_measure_state = 1;
        } else {
            LV_LOG_USER("Stop Heart Rate measurement");
            HeartRate_StopMeasurement();
            key3_measure_state = 0;
        }
    }
    else if (active_page == PAGE_TEMPERATURE) {
        if (key3_measure_state == 0) {
            LV_LOG_USER("Start Temperature measurement");
            Temperature_StartMeasurement();
            key3_measure_state = 1;
        } else {
            LV_LOG_USER("Stop Temperature measurement");
            Temperature_StopMeasurement();
            key3_measure_state = 0;
        }
    }
    else {
        LV_LOG_USER("KEY3 ignored - no measurement page active");
    }
}

// ========== 电源键状态机处理 ==========
static void PowerKey_StateMachine(void)
{
    switch (power_key_state) {
        case KEY_STATE_IDLE:
            // 等待中断信号量
            if (xSemaphoreTake(key_semaphore, 0) == pdTRUE) {
                power_key_state = KEY_STATE_DEBOUNCE;
                shutdown_ui_shown = false;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));
            if (PowerKey_IsPressed()) {
                power_press_start_time = xTaskGetTickCount();
                power_key_state = KEY_STATE_WAIT_RELEASE;
            } else {
                power_key_state = KEY_STATE_IDLE;
            }
            break;

        case KEY_STATE_WAIT_RELEASE:
            // 检测是否松开（短按）
            if (!PowerKey_IsPressed()) {
                // 短按 - 可以添加功能，如返回首页
                LV_LOG_USER("Power key short press");
                power_key_state = KEY_STATE_IDLE;
            }
            // 检测长按
            else if ((xTaskGetTickCount() - power_press_start_time) >= pdMS_TO_TICKS(KEY_LONG_PRESS_MS)) {
                if (!shutdown_ui_shown) {
                    LV_LOG_USER("Long press detected - showing power off UI");
                    GUI_Load_PowerPage();
                    shutdown_ui_shown = true;
                }
                // 继续在此状态，等待用户操作关机界面
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            break;

        case KEY_STATE_LONG_PRESS_DONE:
            power_key_state = KEY_STATE_IDLE;
            break;
    }
}

// ========== 主按键检测任务 ==========
void Key_Detect_Task(void *pvParameters)
{
    const TickType_t measure_key_scan_interval = pdMS_TO_TICKS(20);
    const TickType_t debounce_delay = pdMS_TO_TICKS(30);

    LV_LOG_USER("Key detect task started");

    for (;;) {
        // 1. 处理电源键状态机
        PowerKey_StateMachine();

        // 2. 扫描测量按键（只在非关机界面时响应）
        if (!shutdown_ui_shown) {
            for (uint8_t i = 0; i < MEASURE_KEY_COUNT; i++) {
                if (Key_IsPressed(&measure_keys[i])) {
                    vTaskDelay(debounce_delay);

                    if (Key_IsPressed(&measure_keys[i])) {
                        // 等待释放
                        while (Key_IsPressed(&measure_keys[i])) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                        }

                        // 执行对应操作
                        switch (measure_keys[i].id) {
                            case KEY1: Handle_Key1(); break;
                            case KEY2: Handle_Key2(); break;
                            case KEY3: Handle_Key3(); break;
                            default: break;
                        }

                        vTaskDelay(pdMS_TO_TICKS(100)); // 防连击
                    }
                }
            }
        }

        vTaskDelay(measure_key_scan_interval);
    }
}

// ========== 兼容层函数 ==========
KeyNum_t Key_GetNum(void) {
    for (uint8_t i = 0; i < MEASURE_KEY_COUNT; i++) {
        if (Key_IsPressed(&measure_keys[i])) {
            Key_DelayMs(20);
            while (Key_IsPressed(&measure_keys[i]));
            Key_DelayMs(20);
            return measure_keys[i].id;
        }
    }
    return KEY_NONE;
}

KeyNum_t Key_Scan(void) {
    for (uint8_t i = 0; i < MEASURE_KEY_COUNT; i++) {
        if (Key_IsPressed(&measure_keys[i])) {
            return measure_keys[i].id;
        }
    }
    return KEY_NONE;
}

void Key_ResetState(void) {
    active_page = PAGE_NONE;
    key3_measure_state = 0;
    shutdown_ui_shown = false;
    power_key_state = KEY_STATE_IDLE;
}

void Key_SetActivePage(uint8_t page_type) {
    active_page = (ActivePage_t)page_type;
    key3_measure_state = 0;
}

// 电源键强制重置（供关机界面调用）
void Key_ResetPowerState(void) {
    shutdown_ui_shown = false;
    power_key_state = KEY_STATE_IDLE;
}