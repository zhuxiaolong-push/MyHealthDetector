#include "App_task.h"
#include "touch.h"
#include "Com_Delay.h"
#include "key.h"

//启动任务
#define START_TASK_STACK 256
#define START_TASK_PRIORITY 1
TaskHandle_t start_task_handle;
void start_task(void *pvParameters);

//按键检测任务（整合电源键和测量按键）
#define KEY_DETECT_TASK_STACK 512
#define KEY_DETECT_TASK_PRIORITY 6
TaskHandle_t key_detect_task_handle;

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

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

void App_task_init(void)
{
    //初始化
    GUI_Init();
    Key_Init();
    APP_AutoSleep_Init();
    Backlight_Init(); 
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

    xTaskCreate((TaskFunction_t)Gui_task,
                (char *)"Gui_task",
                (configSTACK_DEPTH_TYPE)Gui_task_STACK,
                (void *)NULL,
                (UBaseType_t)Gui_task_PRIORITY,
                (TaskHandle_t *)&Gui_task_handle);

    // 创建按键检测任务（整合电源键 + 测量按键）
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

    taskEXIT_CRITICAL();

    vTaskDelete(NULL);
}

void Gui_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(5);  // 5ms周期

    while(1)
    {
        lv_timer_handler();

        // 更新心率测量状态
        extern void HeartRate_Update(void);
        HeartRate_Update();

        // 更新体温测量状态
        extern void Temperature_Update(void);
        Temperature_Update();

        vTaskDelayUntil(&xLastWakeTime, xPeriod); 
    }
}

//自动休眠任务
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
