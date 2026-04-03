#include "APP_AutoSleep.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"
#include "backlight.h"

/* ==================== 配置参数 ==================== */
#define AUTO_SLEEP_TIMEOUT_MS   60000   /* 无操作1分钟后睡眠 */

/* ==================== 静态变量 ==================== */
static TickType_t last_activity_time = 0;
static bool auto_sleep_enabled = true;  /* 默认开启 */
static bool is_sleeping = false;

/* ==================== 公共函数实现 ==================== */

void APP_AutoSleep_Init(void)
{
    last_activity_time = xTaskGetTickCount();
    is_sleeping = false;
    auto_sleep_enabled = false;
    LV_LOG_USER("APP_AutoSleep initialized, timeout=%ds", AUTO_SLEEP_TIMEOUT_MS/1000);
}

void APP_AutoSleep_ResetTimer(void)
{
    last_activity_time = xTaskGetTickCount();

    /* 如果正在睡眠中，唤醒屏幕 */
    if (is_sleeping) {
        APP_AutoSleep_WakeUp();
    }
}

void APP_AutoSleep_SetEnable(bool enable)
{
    auto_sleep_enabled = enable;
    LV_LOG_USER("Auto Sleep %s", enable ? "Enabled" : "Disabled");

    if (enable) {
        last_activity_time = xTaskGetTickCount();
    }
}

bool APP_AutoSleep_IsEnabled(void)
{
    return auto_sleep_enabled;
}

bool APP_AutoSleep_IsSleeping(void)
{
    return is_sleeping;
}

TickType_t APP_AutoSleep_GetLastActivityTime(void)
{
    return last_activity_time;
}

void APP_AutoSleep_EnterSleep(void)
{
    if (is_sleeping) return;

    LV_LOG_USER("Entering sleep mode...");
    is_sleeping = true;

    /* 关闭屏幕背光 */
    Backlight_Off();
}

void APP_AutoSleep_WakeUp(void)
{
    if (!is_sleeping) return;

    LV_LOG_USER("Waking up...");
    is_sleeping = false;

    /* 恢复屏幕背光 */
    Backlight_On();

    /* 重置计时器 */
    last_activity_time = xTaskGetTickCount();
}
