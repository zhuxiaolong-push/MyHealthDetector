#ifndef __GUI_MAIN_H__
#define __GUI_MAIN_H__

#include "lvgl.h"
#include "GUI_about.h"
#include "GUI_Common.h"
#include "GUI_Settings.h"
#include "GUI_HeartRate.h"
#include "GUI_Temperature.h"

/* ==================== 公共函数声明 ==================== */

void GUI_Init(void);
void GUI_Main_page_Init(void);
void GUI_Load_HomePage(void);

#endif /* __GUI_MAIN_H__ */

