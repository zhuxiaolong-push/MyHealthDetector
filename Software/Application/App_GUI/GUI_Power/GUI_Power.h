#ifndef __GUI_POWER_H__
#define __GUI_POWER_H__

#include "lvgl.h"
#include "App_task.h"
#include "GUI_Common.h"

#define COLOR_SLIDER_TRACK  lv_color_hex(0x2a3f5f)
#define COLOR_SLIDER_FILL   lv_color_hex(0xE94560)

#define SLIDER_WIDTH        200
#define SLIDER_HEIGHT       50
#define KNOB_SIZE           40

/* ==================== 公共函数声明 ==================== */
void GUI_Power_page_Init(void);
void GUI_Load_PowerPage(void);
void GUI_Unload_PowerPage(void);
lv_obj_t* GUI_Get_PowerPage(void);

#endif /* __GUI_POWER_H__ */
