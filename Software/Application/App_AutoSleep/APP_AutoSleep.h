#ifndef __APP_AUTO_SLEEP_H__
#define __APP_AUTO_SLEEP_H__

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"

/* ==================== 自动睡眠模块接口 ==================== */

void APP_AutoSleep_Init(void);
void APP_AutoSleep_ResetTimer(void);
void APP_AutoSleep_SetEnable(bool enable);
bool APP_AutoSleep_IsEnabled(void);
bool APP_AutoSleep_IsSleeping(void);
TickType_t APP_AutoSleep_GetLastActivityTime(void);
void APP_AutoSleep_EnterSleep(void);
void APP_AutoSleep_WakeUp(void);

#endif /* __APP_AUTO_SLEEP_H__ */
