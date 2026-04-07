#include "App_task.h"
#include "touch.h"
#include "Com_Delay.h"
#include "key.h"
#include "GUI_HeartRate.h"
#include "GUI_Temperature.h"
#include "GUI_Power.h"
#include "GUI_main.h"

//启动任务
#define START_TASK_STACK 256
#define START_TASK_PRIORITY 1
TaskHandle_t start_task_handle;
void start_task(void *pvParameters);

//按键检测任务（已集成到这里）
#define KEY_DETECT_TASK_STACK 512
#define KEY_DETECT_TASK_PRIORITY 6
TaskHandle_t key_detect_task_handle;
void Key_Detect_Task(void *pvParameters);  // 前向声明

//显示任务
#define Gui_task_STACK 1024
#define Gui_task_PRIORITY 5
TaskHandle_t Gui_task_handle;
void Gui_task(void *pvParameters);

//自动休眠任务
#define AUTO_SLEEP_TASK_STACK   256
#define AUTO_SLEEP_TASK_PRIORITY 4
TaskHandle_t AutoSleep_handle;
void AutoSleep_task(void *pvParameters);

//背光更新任务
#define BACKLIGHT_TASK_STACK    256
#define BACKLIGHT_TASK_PRIORITY 2
TaskHandle_t backlight_task_handle;
void Backlight_task(void *pvParameters);

//背光控制队列
QueueHandle_t backlight_queue = NULL;

// 电源键信号量（Key.c 中会引用）
SemaphoreHandle_t key_semaphore = NULL;

// ========== 按键状态管理 ==========
static volatile uint8_t active_page = KEY_PAGE_NONE;  // 当前测量页面
static volatile uint8_t key3_measure_state = 0;       // KEY3状态：0=停止, 1=测量中

// 电源键状态机变量
static KeyState_t power_key_state = KEY_STATE_IDLE;
static TickType_t power_press_start_time = 0;
static bool shutdown_ui_shown = false;


static void Handle_Key3_Measurement(void);
static void PowerKey_StateMachine(void);

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

void App_task_init(void)
{
    //初始化
    Key_Init();  // 先初始化按键硬件
    GUI_Init();
    APP_AutoSleep_Init();
    Backlight_Init(); 
    W25Q16_Init();
    
    // 创建信号量
    key_semaphore = xSemaphoreCreateBinary();
    
    xTaskCreate((TaskFunction_t)start_task,               
            (char *)"start_task",                    
            (configSTACK_DEPTH_TYPE)START_TASK_STACK, 
            (void *)NULL,                            
            (UBaseType_t)START_TASK_PRIORITY,         
            (TaskHandle_t *)&start_task_handle);  

    vTaskStartScheduler();
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();

    //创建背光控制队列
    backlight_queue = xQueueCreate(1, sizeof(uint8_t));

    xTaskCreate((TaskFunction_t)Gui_task,
                (char *)"Gui_task",
                (configSTACK_DEPTH_TYPE)Gui_task_STACK,
                (void *)NULL,
                (UBaseType_t)Gui_task_PRIORITY,
                (TaskHandle_t *)&Gui_task_handle);

    // 创建按键检测任务（逻辑全部在此文件）
    xTaskCreate((TaskFunction_t)Key_Detect_Task,
                (char *)"key_detect_task",
                (configSTACK_DEPTH_TYPE)KEY_DETECT_TASK_STACK,
                (void *)NULL,
                (UBaseType_t)KEY_DETECT_TASK_PRIORITY,
                (TaskHandle_t *)&key_detect_task_handle);

    xTaskCreate((TaskFunction_t)AutoSleep_task,
            (char *)"auto_sleep_task",
            (configSTACK_DEPTH_TYPE)AUTO_SLEEP_TASK_STACK,
            (void *)NULL,
            (UBaseType_t)AUTO_SLEEP_TASK_PRIORITY,
            (TaskHandle_t *)&AutoSleep_handle);

    //创建背光更新任务
    xTaskCreate((TaskFunction_t)Backlight_task,
                (char *)"backlight_task",
                (configSTACK_DEPTH_TYPE)BACKLIGHT_TASK_STACK,
                (void *)NULL,
                (UBaseType_t)BACKLIGHT_TASK_PRIORITY,
                (TaskHandle_t *)&backlight_task_handle);

    taskEXIT_CRITICAL();

    vTaskDelete(NULL);
}

// ========== 按键检测任务（完整逻辑集成到这里）==========
void Key_Detect_Task(void *pvParameters)
{
    (void)pvParameters;
    KeyNum_t last_key = KEY_NONE;
    uint8_t debounce_cnt = 0;
    
    LV_LOG_USER("Key Detect Task Started");
    
    while(1) {
        // 处理电源键状态机（长按关机）
        PowerKey_StateMachine();
        
        // 扫描测量按键 KEY1/KEY2/KEY3
        KeyNum_t current_key = Key_Scan();
        
        // 消抖处理
        if (current_key != last_key) {
            if (debounce_cnt < 3) {
                debounce_cnt++;
            } else {
                // 确认按键变化
                if (current_key != KEY_NONE && last_key == KEY_NONE) {
                    // 按键按下
                    switch(current_key) {
                        case KEY1:
                            LV_LOG_USER("KEY1 pressed - Heart Rate Page");
                            active_page = KEY_PAGE_HEART_RATE;
                            key3_measure_state = 0;
                            GUI_Load_HeartRatePage();
                            break;
                            
                        case KEY2:
                            LV_LOG_USER("KEY2 pressed - Temperature Page");
                            active_page = KEY_PAGE_TEMPERATURE;
                            key3_measure_state = 0;
                            GUI_Load_TemperaturePage();
                            break;
                            
                        case KEY3:
                            LV_LOG_USER("KEY3 pressed - Toggle Measurement");
                            Handle_Key3_Measurement();
                            break;
                            
                        default:
                            break;
                    }
                }
                last_key = current_key;
                debounce_cnt = 0;
            }
        } else {
            debounce_cnt = 0;
        }
        
        // 长按检测（如果当前有按键按下，检测是否长按）
        if (current_key != KEY_NONE) {
            // 可以在这里添加长按逻辑
        }
        
        vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz 扫描频率
    }
}

// ========== KEY3 测量控制逻辑 ==========
static void Handle_Key3_Measurement(void)
{
    if (active_page == KEY_PAGE_HEART_RATE) {
        if (key3_measure_state == 0) {
            LV_LOG_USER("Start Heart Rate measurement (MAX30102)");
            HeartRate_StartMeasurement();
            key3_measure_state = 1;
        } else {
            LV_LOG_USER("Stop Heart Rate measurement");
            HeartRate_StopMeasurement();
            key3_measure_state = 0;
        }
    }
    else if (active_page == KEY_PAGE_TEMPERATURE) {
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

// ========== 电源键状态机（集成到这里）==========
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
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            break;

        case KEY_STATE_LONG_PRESS_DONE:
            power_key_state = KEY_STATE_IDLE;
            break;
    }
}

// ========== GUI 任务 ==========
void Gui_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(5);  // 5ms周期

    while(1)
    {
        lv_timer_handler();

        // 更新心率测量（调用真实MAX30102接口）
        extern void HeartRate_Update(void);
        HeartRate_Update();

        // 更新体温测量
        extern void Temperature_Update(void);
        Temperature_Update();

        vTaskDelayUntil(&xLastWakeTime, xPeriod); 
    }
}

// ========== 自动休眠任务 ==========
void AutoSleep_task(void *pvParameters)
{
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        if (!APP_AutoSleep_IsEnabled()) continue;
        if (APP_AutoSleep_IsSleeping()) continue;

        TickType_t idle_time = xTaskGetTickCount() - APP_AutoSleep_GetLastActivityTime();

        if (idle_time >= pdMS_TO_TICKS(60000)) {
            APP_AutoSleep_EnterSleep();
        }
    }
}

// ========== 背光更新任务 ==========
void Backlight_task(void *pvParameters)
{
    uint8_t brightness;
    
    (void)pvParameters;
    
    for (;;) {
        if (xQueueReceive(backlight_queue, &brightness, portMAX_DELAY) == pdTRUE) {
            Backlight_SetBrightness(brightness);
        }
    }
}

void App_SetPage(uint8_t page)
{
    active_page = page;
    if (page == KEY_PAGE_NONE) {
        key3_measure_state = 0;
    }
}
