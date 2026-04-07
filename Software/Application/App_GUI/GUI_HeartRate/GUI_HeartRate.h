#ifndef __GUI_HEARTRATE_H__
#define __GUI_HEARTRATE_H__

#include "lvgl.h"
#include "GUI_Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "GUI_about.h"

/* ==================== 心率测量状态 ==================== */
typedef enum {
    HR_STATE_IDLE,      /* 待机，显示"--" */
    HR_STATE_MEASURING, /* 测量中 */
    HR_STATE_DONE       /* 测量完成，显示数值 */
} HeartRate_State_t;

/* ==================== 公共函数声明 ==================== */
void GUI_HeartRate_page_Init(void);
void GUI_Load_HeartRatePage(void);

/* ==================== 测量控制接口（供key.c调用）==================== */
void HeartRate_StartMeasurement(void);  /* 开始测量 */
void HeartRate_StopMeasurement(void);     /* 停止测量 */
HeartRate_State_t HeartRate_GetState(void);

#endif /* __GUI_HEARTRATE_H__ */
