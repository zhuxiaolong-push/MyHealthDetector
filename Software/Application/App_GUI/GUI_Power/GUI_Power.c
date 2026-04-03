#include "GUI_Power.h"
#include <string.h>
#include "key.h"

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_PowerPage = NULL;
static lv_obj_t *panel = NULL;
static lv_obj_t *btn_cancel = NULL;
static lv_obj_t *btn_confirm = NULL;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void overlay_event_cb(lv_event_t *e);
static void btn_event_cb(lv_event_t *e);
static void execute_power_off(void);
static void cancel_power_off(void);

/* ==================== 公共函数实现 ==================== */

lv_obj_t* GUI_Get_PowerPage(void)
{
    return ui_PowerPage;
}

void GUI_Power_page_Init(void)
{
    if (ui_PowerPage != NULL) return;
    page_init();
}

void GUI_Load_PowerPage(void)
{
    LV_LOG_USER("Loading PowerPage");

    if (ui_PowerPage == NULL) {
        GUI_Power_page_Init();
    }

    lv_disp_load_scr(ui_PowerPage);
}

void GUI_Unload_PowerPage(void)
{
    LV_LOG_USER("Unloading PowerPage");
    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}

/* ==================== 页面创建 - 适配 320x240 横屏 ==================== */
static void page_init(void)
{
    /* 页面根对象 - 320x240 */
    ui_PowerPage = lv_obj_create(NULL);
    lv_obj_set_size(ui_PowerPage, 320, 240);
    lv_obj_set_style_bg_color(ui_PowerPage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_PowerPage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_PowerPage, LV_OBJ_FLAG_SCROLLABLE);

    /* ==================== 半透明遮罩层 ==================== */
    lv_obj_t *overlay = lv_obj_create(ui_PowerPage);
    lv_obj_set_size(overlay, 320, 240);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(overlay, overlay_event_cb, LV_EVENT_CLICKED, NULL);

    /* ==================== 确认面板 - 居中 ==================== */
    #define PANEL_WIDTH     240
    #define PANEL_HEIGHT    140

    panel = lv_obj_create(ui_PowerPage);
    lv_obj_set_size(panel, PANEL_WIDTH, PANEL_HEIGHT);
    lv_obj_center(panel);  /* 在 320x240 屏幕中居中 */
    lv_obj_set_style_bg_color(panel, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_40, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* 电源图标 - 顶部居中 */
    lv_obj_t *icon = lv_label_create(panel);
    lv_label_set_text(icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);

    /* 标题 - 图标下方 */
    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Power Off Device?");
    lv_obj_set_style_text_font(title, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 38);

    /* 说明文字 - 标题下方 */
    lv_obj_t *subtitle = lv_label_create(panel);
    lv_label_set_text(subtitle, "Tap outside to cancel");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 60);

    /* ==================== 双按钮 - 底部居中 ==================== */
    #define BTN_WIDTH       90
   // #define BTN_HEIGHT      34

    /* Cancel 按钮 - 左侧 */
    btn_cancel = lv_btn_create(panel);
    lv_obj_set_size(btn_cancel, BTN_WIDTH, BTN_HEIGHT);
    /* 在面板内居中偏左：(面板宽240 - 两个按钮宽 - 间距15) / 2 = (240-90-90-15)/2 = 22.5 ≈ 22 */
    lv_obj_set_pos(btn_cancel, 17, 86);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x4a4a5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x5a5a6a), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_cancel, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cancel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_cancel, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_set_style_text_font(label_cancel, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_cancel, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(label_cancel);

    /* Confirm 按钮 - 右侧 */
    btn_confirm = lv_btn_create(panel);
    lv_obj_set_size(btn_confirm, BTN_WIDTH, BTN_HEIGHT);
    /* 右侧按钮位置：左侧按钮位置 + 按钮宽 + 间距 = 22 + 90 + 15 = 127 */
    lv_obj_set_pos(btn_confirm, 122, 86);
    lv_obj_set_style_bg_color(btn_confirm, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_confirm, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_confirm, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_confirm, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_confirm, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_confirm, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "Confirm");
    lv_obj_set_style_text_font(label_confirm, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_confirm, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(label_confirm);
}

/* ==================== 事件回调 ==================== */

static void overlay_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current = lv_event_get_current_target(e);

    if (target == current) {
        LV_LOG_USER("Cancel power off - overlay clicked");
        cancel_power_off();
    }
}

static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    if (btn == btn_cancel) {
        LV_LOG_USER("Cancel button clicked");
        cancel_power_off();
    }
    else if (btn == btn_confirm) {
        LV_LOG_USER("Confirm button clicked - powering off");
        execute_power_off();
    }
}

void Power_Off_Hardware(void)
{
  GPIO_ResetBits(POWER_CTRL_PORT, POWER_CTRL_PIN);
}

static void execute_power_off(void)
{
    LV_LOG_USER("Executing power off sequence");

    lv_obj_clean(panel);

    lv_obj_t *icon = lv_label_create(panel);
    lv_label_set_text(icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -15);

    lv_obj_t *powering_off = lv_label_create(panel);
    lv_label_set_text(powering_off, "Powering off...");
    lv_obj_set_style_text_font(powering_off, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(powering_off, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(powering_off, LV_ALIGN_CENTER, 0, 20);

    lv_refr_now(lv_disp_get_default());

    for (volatile int i = 0; i < 500000; i++);
    
    Key_ResetPowerState();  // 重置电源键状态
    Power_Off_Hardware();
}

static void cancel_power_off(void)
{
    LV_LOG_USER("Power off cancelled");
    Key_ResetPowerState();  // 重置电源键状态，允许再次触发
    GUI_Unload_PowerPage();
}
