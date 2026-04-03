#ifndef __GUI_ABOUT_H__
#define __GUI_ABOUT_H__

#include "lvgl.h"
#include "GUI_Common.h"

#define BACK_BTN_SIZE       28

#define COLOR_SLIDER_TRACK  lv_color_hex(0x2a3f5f)
#define COLOR_SLIDER_FILL   lv_color_hex(0xE94560)

#define SLIDER_WIDTH        200
#define SLIDER_HEIGHT       50
#define KNOB_SIZE           40

/* ==================== 公共函数声明 ==================== */
void GUI_About_page_Init(void);
void GUI_Load_AboutPage(void);

#endif /* __GUI_ABOUT_H__ */

