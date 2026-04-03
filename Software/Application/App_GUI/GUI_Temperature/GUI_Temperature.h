#ifndef __GUI_TEMPERATURE_H__
#define __GUI_TEMPERATURE_H__

#include "lvgl.h"
#include "GUI_Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "GUI_about.h"

/* ==================== 体温测量状态 ==================== */
typedef enum {
    TEMP_STATE_IDLE,      /* 待机 */
    TEMP_STATE_MEASURING, /* 测量中 */
    TEMP_STATE_DONE       /* 完成 */
} Temperature_State_t;

/* ==================== 体温结果 ==================== */
typedef enum {
    TEMP_LOW,       /* 低温 */
    TEMP_NORMAL,    /* 正常 */
    TEMP_LOW_FEVER, /* 低烧 */
    TEMP_HIGH_FEVER /* 高烧 */
} Temperature_Result_t;

/* ==================== 公共函数 ==================== */
void GUI_Temperature_page_Init(void);
void GUI_Load_TemperaturePage(void);
void Temperature_StartMeasurement(void);
void Temperature_StopMeasurement(void);
Temperature_State_t Temperature_GetState(void);
void Temperature_Update(void);

#endif /* __GUI_TEMPERATURE_H__ */
