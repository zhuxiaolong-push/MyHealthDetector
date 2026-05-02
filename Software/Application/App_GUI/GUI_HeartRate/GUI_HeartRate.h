#ifndef __GUI_HEARTRATE_H__
#define __GUI_HEARTRATE_H__

#include "lvgl.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef BACK_BTN_SIZE
#define BACK_BTN_SIZE     32
#endif

#ifndef STATUS_BAR_HEIGHT
#define STATUS_BAR_HEIGHT 30
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE       500
#endif

#ifndef COLOR_BG
#define COLOR_BG          lv_color_hex(0x1a1a2e)
#define COLOR_STATUS_BAR  lv_color_hex(0x16213e)
#define COLOR_PANEL_BG    lv_color_hex(0x0f3460)
#define COLOR_MENU_BTN    lv_color_hex(0xe94560)
#define COLOR_MENU_BTN_PR lv_color_hex(0xff6b6b)
#define COLOR_TEXT_LIGHT  lv_color_hex(0xffffff)
#define COLOR_TEXT_GRAY   lv_color_hex(0xa0a0a0)
#define FONT_TITLE        &lv_font_montserrat_16
#define FONT_MENU         &lv_font_montserrat_14
#define FONT_HINT         &lv_font_montserrat_12
#endif

typedef enum {
    HR_STATE_IDLE,
    HR_STATE_MEASURING,
    HR_STATE_DONE
} HeartRate_State_t;

void GUI_HeartRate_page_Init(void);
void GUI_Load_HeartRatePage(void);

void HeartRate_StartMeasurement(void);
void HeartRate_StopMeasurement(void);
HeartRate_State_t HeartRate_GetState(void);
uint8_t HeartRate_GetValue(void);

void HeartRate_Update(void);

#endif /* __GUI_HEARTRATE_H__ */