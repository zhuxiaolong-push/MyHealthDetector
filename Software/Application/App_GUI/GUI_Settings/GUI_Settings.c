#include "GUI_Settings.h"
#include <string.h>
#include "backlight.h"

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_SettingsPage = NULL;
static lv_obj_t *back_btn = NULL;

/* 设置项对象 */
static lv_obj_t *slider_brightness = NULL;
static lv_obj_t *sw_wifi = NULL;
static lv_obj_t *sw_sound = NULL;
static lv_obj_t *sw_auto_sleep = NULL;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void slider_event_cb(lv_event_t *e);
static void switch_event_cb(lv_event_t *e);
//static void create_setting_item(lv_obj_t *parent, const char *label, lv_obj_t **control, uint8_t type, uint8_t y_pos);

/* ==================== 公共函数实现 ==================== */
void GUI_Settings_page_Init(void)
{
    if (ui_SettingsPage != NULL) return;
    page_init();
}

void GUI_Load_SettingsPage(void)
{
    if (ui_SettingsPage == NULL) {
        GUI_Settings_page_Init();
    }
    lv_disp_load_scr(ui_SettingsPage);
}

/* ==================== 页面创建 ==================== */
static void page_init(void)
{
    /* 页面根对象 - 320x240 */
    ui_SettingsPage = lv_obj_create(NULL);
    lv_obj_set_size(ui_SettingsPage, 320, 240);
    lv_obj_set_style_bg_color(ui_SettingsPage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_SettingsPage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_SettingsPage, LV_OBJ_FLAG_SCROLLABLE);

    /* ==================== 状态栏 ==================== */
    lv_obj_t *status_bar = lv_obj_create(ui_SettingsPage);
    lv_obj_set_size(status_bar, 320, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, COLOR_STATUS_BAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t *title = lv_label_create(status_bar);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* ==================== 回退按钮 ==================== */
    back_btn = lv_btn_create(ui_SettingsPage);
    lv_obj_set_size(back_btn, 32, 32);
    lv_obj_set_pos(back_btn, 5, STATUS_BAR_HEIGHT + 5);
    lv_obj_set_style_bg_color(back_btn, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back_btn, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_label_create(back_btn);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_icon, FONT_BACK_BTN, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_icon, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(back_icon);

    /* ==================== 设置项容器 ==================== */
    lv_obj_t *container = lv_obj_create(ui_SettingsPage);
    lv_obj_set_size(container, 310, 240 - STATUS_BAR_HEIGHT - 45);  /* 留45px给返回按钮 */
    lv_obj_set_pos(container, 5, STATUS_BAR_HEIGHT + 42);
    lv_obj_set_style_bg_color(container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 8, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);  /* 禁止滚动 */

    /* 设置项高度和间距计算 */
    #define ITEM_HEIGHT     34
    #define ITEM_SPACING    8
    #define LABEL_WIDTH     100

    /* 1. 亮度调节 - 滑动条 */
    lv_obj_t *item1 = lv_obj_create(container);
    lv_obj_set_size(item1, 290, ITEM_HEIGHT);
    lv_obj_set_pos(item1, 0, 0);
    lv_obj_set_style_bg_color(item1, lv_color_hex(0x1a2a4a), LV_PART_MAIN);
    lv_obj_set_style_radius(item1, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(item1, 0, LV_PART_MAIN);
    lv_obj_clear_flag(item1, LV_OBJ_FLAG_SCROLLABLE);

    /* 标签 */
    lv_obj_t *label1 = lv_label_create(item1);
    lv_label_set_text(label1, LV_SYMBOL_IMAGE " Brightness");
    lv_obj_set_style_text_font(label1, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label1, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label1, LV_ALIGN_LEFT_MID, 10, 0);

    /* 滑动条 */
    slider_brightness = lv_slider_create(item1);
    lv_obj_set_size(slider_brightness, 140, 8);
    lv_obj_align(slider_brightness, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_slider_set_range(slider_brightness, 10, 100);
    lv_slider_set_value(slider_brightness, 80, LV_ANIM_OFF);
    uint8_t current_brightness = Backlight_GetBrightness();
    lv_slider_set_value(slider_brightness, current_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_brightness, lv_color_hex(0x2d3d5d), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_brightness, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider_brightness, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(slider_brightness, 4, LV_PART_INDICATOR);
    lv_obj_set_style_pad_hor(slider_brightness, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_ver(slider_brightness, 6, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider_brightness, COLOR_TEXT_LIGHT, LV_PART_KNOB);
    lv_obj_set_style_radius(slider_brightness, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_brightness, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 2. WIFI开关 */
    lv_obj_t *item2 = lv_obj_create(container);
    lv_obj_set_size(item2, 290, ITEM_HEIGHT);
    lv_obj_set_pos(item2, 0, ITEM_HEIGHT + ITEM_SPACING);
    lv_obj_set_style_bg_color(item2, lv_color_hex(0x1a2a4a), LV_PART_MAIN);
    lv_obj_set_style_radius(item2, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(item2, 0, LV_PART_MAIN);
    lv_obj_clear_flag(item2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label2 = lv_label_create(item2);
    lv_label_set_text(label2, LV_SYMBOL_WIFI " WiFi");
    lv_obj_set_style_text_font(label2, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label2, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 10, 0);

    sw_wifi = lv_switch_create(item2);
    lv_obj_align(sw_wifi, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_size(sw_wifi, 44, 22);
    lv_obj_set_style_bg_color(sw_wifi, lv_color_hex(0x3a4a6a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_wifi, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_add_state(sw_wifi, LV_STATE_CHECKED);  /* 默认打开 */
    lv_obj_add_event_cb(sw_wifi, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 3. 提示音开关 */
    lv_obj_t *item3 = lv_obj_create(container);
    lv_obj_set_size(item3, 290, ITEM_HEIGHT);
    lv_obj_set_pos(item3, 0, (ITEM_HEIGHT + ITEM_SPACING) * 2);
    lv_obj_set_style_bg_color(item3, lv_color_hex(0x1a2a4a), LV_PART_MAIN);
    lv_obj_set_style_radius(item3, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(item3, 0, LV_PART_MAIN);
    lv_obj_clear_flag(item3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label3 = lv_label_create(item3);
    lv_label_set_text(label3, LV_SYMBOL_VOLUME_MAX " Sound");
    lv_obj_set_style_text_font(label3, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label3, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label3, LV_ALIGN_LEFT_MID, 10, 0);

    sw_sound = lv_switch_create(item3);
    lv_obj_align(sw_sound, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_size(sw_sound, 44, 22);
    lv_obj_set_style_bg_color(sw_sound, lv_color_hex(0x3a4a6a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_sound, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_add_event_cb(sw_sound, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 4. 自动休眠开关 */
    lv_obj_t *item4 = lv_obj_create(container);
    lv_obj_set_size(item4, 290, ITEM_HEIGHT);
    lv_obj_set_pos(item4, 0, (ITEM_HEIGHT + ITEM_SPACING) * 3);
    lv_obj_set_style_bg_color(item4, lv_color_hex(0x1a2a4a), LV_PART_MAIN);
    lv_obj_set_style_radius(item4, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(item4, 0, LV_PART_MAIN);
    lv_obj_clear_flag(item4, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label4 = lv_label_create(item4);
    lv_label_set_text(label4, LV_SYMBOL_EYE_OPEN " Auto Sleep");
    lv_obj_set_style_text_font(label4, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label4, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label4, LV_ALIGN_LEFT_MID, 10, 0);

    sw_auto_sleep = lv_switch_create(item4);
    lv_obj_align(sw_auto_sleep, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_size(sw_auto_sleep, 44, 22);
    lv_obj_set_style_bg_color(sw_auto_sleep, lv_color_hex(0x3a4a6a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_auto_sleep, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_add_state(sw_auto_sleep, LV_STATE_CHECKED);  /* 默认打开 */
    lv_obj_add_event_cb(sw_auto_sleep, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 5. 版本信息（静态文本，填充剩余空间） */
    lv_obj_t *item5 = lv_obj_create(container);
    lv_obj_set_size(item5, 290, ITEM_HEIGHT);
    lv_obj_set_pos(item5, 0, (ITEM_HEIGHT + ITEM_SPACING) * 4);
    lv_obj_set_style_bg_color(item5, lv_color_hex(0x1a2a4a), LV_PART_MAIN);
    lv_obj_set_style_radius(item5, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(item5, 0, LV_PART_MAIN);
    lv_obj_clear_flag(item5, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label5 = lv_label_create(item5);
    lv_label_set_text(label5, LV_SYMBOL_SETTINGS " Version");
    lv_obj_set_style_text_font(label5, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label5, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label5, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *ver_val = lv_label_create(item5);
    lv_label_set_text(ver_val, "v1.0.0");
    lv_obj_set_style_text_font(ver_val, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(ver_val, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(ver_val, LV_ALIGN_RIGHT_MID, -10, 0);
}

/* ==================== 事件回调 ==================== */

static void back_event_cb(lv_event_t *e)
{
    LV_LOG_USER("Back to HomePage");
    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    LV_LOG_USER("Brightness changed to %d%%", val);
    // 设置背光亮度
    Backlight_SetBrightness((uint8_t)val);
}

static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    lv_state_t state = lv_obj_get_state(sw);
    bool checked = (state & LV_STATE_CHECKED);

    if (sw == sw_wifi) {
        LV_LOG_USER("WiFi %s", checked ? "ON" : "OFF");
        /* 实际的WIFI开关代码 */
    }
    else if (sw == sw_sound) {
        LV_LOG_USER("Sound %s", checked ? "ON" : "OFF");
        /* 实际的提示音开关代码 */
    }
    else if (sw == sw_auto_sleep) {
        LV_LOG_USER("Auto Sleep %s", checked ? "ON" : "OFF");
        /* 实际的自动休眠开关代码 */
        APP_AutoSleep_SetEnable(checked);
    }
}