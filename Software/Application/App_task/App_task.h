#ifndef APP_TASK_H
#define APP_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "Com_Debug.h"
#include "GUI_main.h"
#include "semphr.h"
#include "GUI_Power.h"
#include "APP_AutoSleep.h"
#include "backlight.h"

void App_task_init(void);

#endif // APP_TASK_H
