#ifndef __GUI_COMMON_H__
#define __GUI_COMMON_H__

#include "lvgl.h"

/* ==================== 暗色主题颜色定义（全局统一）==================== */
#define COLOR_BG            lv_color_hex(0x1A1A2E)   /* 深蓝黑背景 */
#define COLOR_STATUS_BAR    lv_color_hex(0x16213E)   /* 状态栏深蓝 */
#define COLOR_PANEL_BG      lv_color_hex(0x0F3460)   /* 面板背景蓝 */
#define COLOR_MENU_BTN      lv_color_hex(0xE94560)   /* 按钮珊瑚红 */
#define COLOR_MENU_BTN_PR   lv_color_hex(0xC73E54)   /* 按钮按下深红 */
#define COLOR_ACCENT        lv_color_hex(0x533483)   /* 强调紫 */
#define COLOR_TEXT_LIGHT    lv_color_hex(0xFFFFFF)   /* 白色文字 */
#define COLOR_TEXT_GRAY     lv_color_hex(0xB8B8B8)   /* 灰色文字 */

/* ==================== 字体定义（全局统一）==================== */
#define FONT_STATUS         &lv_font_montserrat_14
#define FONT_MENU           &lv_font_montserrat_14
#define FONT_TITLE          &lv_font_montserrat_16
#define FONT_INFO_TITLE     &lv_font_montserrat_16
#define FONT_INFO_TEXT      &lv_font_montserrat_12
#define FONT_INFO_LABEL     &lv_font_montserrat_12
#define FONT_INFO_VALUE     &lv_font_montserrat_14
#define FONT_BACK_BTN       &lv_font_montserrat_16
#define FONT_HINT           &lv_font_montserrat_12

/* ==================== 布局常量（全局统一）==================== */
#define STATUS_BAR_HEIGHT   28
#define MENU_WIDTH          130
#define PADDING             8
#define BTN_HEIGHT          32
#define BTN_SPACING         6
#define INFO_ITEM_HEIGHT    32
#define INFO_ITEM_SPACING   8

/* ==================== 菜单数量 ==================== */
#define MENU_COUNT          4

#endif /* __GUI_COMMON_H__ */

