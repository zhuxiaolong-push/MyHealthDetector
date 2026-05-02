#include "Key.h"
#include "GUI_HeartRate.h"
#include "GUI_Temperature.h"
#include "lvgl.h"

SemaphoreHandle_t key_semaphore = NULL;
static volatile Key_Page_t current_page = KEY_PAGE_NONE;

/* 电源键长按检测状态 */
static volatile uint32_t power_key_press_tick = 0;
static volatile uint8_t  power_key_pressed  = 0;
static volatile uint8_t  power_key_long_done = 0;

static void Key_Delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能 GPIOA 时钟 (KEY1: PA15)
    RCC_AHB1PeriphClockCmd(KEY1_CLK, ENABLE);
    // 使能 GPIOC 时钟 (KEY2: PC6, KEY3: PC7)
    RCC_AHB1PeriphClockCmd(KEY2_CLK, ENABLE);
    
    // 配置 KEY1 (PA15) - 上拉输入
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY1_PORT, &GPIO_InitStructure);
    
    // 配置 KEY2 (PC6) - 上拉输入
    GPIO_InitStructure.GPIO_Pin = KEY2_PIN;
    GPIO_Init(KEY2_PORT, &GPIO_InitStructure);
    
    // 配置 KEY3 (PC7) - 上拉输入
    GPIO_InitStructure.GPIO_Pin = KEY3_PIN;
    GPIO_Init(KEY3_PORT, &GPIO_InitStructure);
    
    // 创建信号量（如果需要）
    key_semaphore = xSemaphoreCreateBinary();
}

// 通用按键扫描函数
static uint8_t Key_Scan(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin)
{
    if (GPIO_ReadInputDataBit(GPIO_Port, GPIO_Pin) == Bit_RESET) {
        Key_Delay(20);  // 消抖
        if (GPIO_ReadInputDataBit(GPIO_Port, GPIO_Pin) == Bit_RESET) {
            // 等待按键释放（带小延时，防止 FreeRTOS 忙等）
            while (GPIO_ReadInputDataBit(GPIO_Port, GPIO_Pin) == Bit_RESET) {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            Key_Delay(20);
            return 1;
        }
    }
    return 0;
}

// KEY3 测量控制：根据当前页面执行对应测量
void Handle_Key3_Measurement(void)
{
    Key_Page_t page = Key_GetCurrentPage();
    
    switch (page) {
        case KEY_PAGE_HEARTRATE: {
            HeartRate_State_t hr_state = HeartRate_GetState();
            
            if (hr_state == HR_STATE_MEASURING) {
                HeartRate_StopMeasurement();
                LV_LOG_USER("KEY3: HeartRate measurement STOPPED");
            } else {
                HeartRate_StartMeasurement();
                LV_LOG_USER("KEY3: HeartRate measurement STARTED");
            }
            break;
        }
            
        case KEY_PAGE_TEMPERATURE: {
            Temperature_State_t temp_state = Temperature_GetState();
            
            if (temp_state == TEMP_STATE_MEASURING) {
                Temperature_StopMeasurement();
                LV_LOG_USER("KEY3: Temperature measurement STOPPED");
            } else {
                Temperature_StartMeasurement();
                LV_LOG_USER("KEY3: Temperature measurement STARTED");
            }
            break;
        }
            
        default:
            LV_LOG_USER("KEY3: No measurement function on current page (page=%d)", page);
            break;
    }
}

// 电源状态重置
void Key_ResetPowerState(void)
{
    current_page = KEY_PAGE_NONE;
    
    // 如果正在测量，停止测量
    if (HeartRate_GetState() == HR_STATE_MEASURING) {
        HeartRate_StopMeasurement();
    }
    if (Temperature_GetState() == TEMP_STATE_MEASURING) {
        Temperature_StopMeasurement();
    }
    
    // 重置电源键长按状态，允许再次触发
    power_key_pressed  = 0;
    power_key_long_done = 0;
    
    LV_LOG_USER("Power state reset");
}

// 按键检测任务
void Key_Detect_Task(void *pvParameters)
{
    (void)pvParameters;
    
    LV_LOG_USER("Key task started");
    
    while (1) {
        Key_Page_t page = Key_GetCurrentPage();
        
        /* ========== 电源键长按检测（独立处理，不阻塞） ========== */
        if (GPIO_ReadInputDataBit(POWER_KEY_PORT, POWER_KEY_PIN) == Bit_RESET) {
            if (!power_key_pressed) {
                power_key_pressed = 1;
                power_key_press_tick = xTaskGetTickCount();
                power_key_long_done = 0;
            } else if (!power_key_long_done) {
                if ((xTaskGetTickCount() - power_key_press_tick) > pdMS_TO_TICKS(2000)) {
                    if (page != KEY_PAGE_POWER) {
                        power_key_long_done = 1;
                        LV_LOG_USER("Power key LONG press detected -> Enter Power Page");
                        Key_SetActivePage(KEY_PAGE_POWER);
                        extern void GUI_Load_PowerPage(void);
                        GUI_Load_PowerPage();
                    }
                }
            }
        } else {
            power_key_pressed = 0;
            power_key_long_done = 0;
        }
        
        /* ========== 在电源页面时，屏蔽 KEY1/KEY2/KEY3 ========== */
        if (page == KEY_PAGE_POWER) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        // KEY1 - 进入心率页面 (PA15)
        if (Key_Scan(KEY1_PORT, KEY1_PIN)) {
            LV_LOG_USER("KEY1: Enter HeartRate Page");
            Key_SetActivePage(KEY_PAGE_HEARTRATE);
            extern void GUI_Load_HeartRatePage(void);
            GUI_Load_HeartRatePage();
            extern void APP_AutoSleep_ResetTimer(void);
            APP_AutoSleep_ResetTimer();
        }
        
        // KEY2 - 进入体温页面 (PC6)
        if (Key_Scan(KEY2_PORT, KEY2_PIN)) {
            LV_LOG_USER("KEY2: Enter Temperature Page");
            Key_SetActivePage(KEY_PAGE_TEMPERATURE);
            extern void GUI_Load_TemperaturePage(void);
            GUI_Load_TemperaturePage();
            extern void APP_AutoSleep_ResetTimer(void);
            APP_AutoSleep_ResetTimer();
        }
        
        // KEY3 - 测量/停止测量 (PC7)
        if (Key_Scan(KEY3_PORT, KEY3_PIN)) {
            LV_LOG_USER("KEY3: Toggle Measurement");
            Handle_Key3_Measurement();
            extern void APP_AutoSleep_ResetTimer(void);
            APP_AutoSleep_ResetTimer();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 扫描间隔
    }
}

// 设置当前活动页面
void Key_SetActivePage(uint8_t page)
{
    current_page = (Key_Page_t)page;
}

// 获取当前活动页面
Key_Page_t Key_GetCurrentPage(void)
{
    return current_page;
}

// 重置按键状态
void Key_ResetState(void)
{
    current_page = KEY_PAGE_NONE;
    
    // 停止所有测量
    if (HeartRate_GetState() == HR_STATE_MEASURING) {
        HeartRate_StopMeasurement();
    }
    if (Temperature_GetState() == TEMP_STATE_MEASURING) {
        Temperature_StopMeasurement();
    }
}

/* ========== 电源键中断服务函数（清除中断标志，防止卡死） ========== */
void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(POWER_KEY_EXTI_LINE) != RESET)
    {
        EXTI_ClearITPendingBit(POWER_KEY_EXTI_LINE);
    }
}