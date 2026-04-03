#include "GUI_HeartRate.h"
#include "GUI_about.h"
#include "key.h"
#include <string.h>
#include <stdio.h>

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_HeartRatePage = NULL;
static lv_obj_t *label_hr_value = NULL;
static lv_obj_t *label_hr_unit = NULL;
static lv_obj_t *label_status = NULL;
static lv_obj_t *label_hint = NULL;
static lv_obj_t *btn_back = NULL;

/* 测量状态 */
static volatile HeartRate_State_t hr_state = HR_STATE_IDLE;
static volatile uint8_t hr_value = 0;
static volatile uint32_t measure_start_time = 0;
static uint8_t measure_counter = 0;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static uint8_t simulate_hr_reading(void);

/* ==================== 公共函数实现 ==================== */

void GUI_HeartRate_page_Init(void)
{
    if (ui_HeartRatePage != NULL) return;
    page_init();
}

void GUI_Load_HeartRatePage(void)
{
    if (ui_HeartRatePage == NULL) {
        GUI_HeartRate_page_Init();
    }

    /* 重置状态 */
    hr_state = HR_STATE_IDLE;
    hr_value = 0;
    update_display();

    lv_disp_load_scr(ui_HeartRatePage);
}

/* ==================== 测量控制接口 ==================== */

void HeartRate_StartMeasurement(void)
{
    if (hr_state == HR_STATE_MEASURING) return;

    hr_state = HR_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    measure_counter = 0;

    LV_LOG_USER("Heart Rate measurement started");
    update_display();
}

void HeartRate_StopMeasurement(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    hr_state = HR_STATE_IDLE;
    hr_value = 0;

    LV_LOG_USER("Heart Rate measurement stopped");
    update_display();
}

HeartRate_State_t HeartRate_GetState(void)
{
    return hr_state;
}

/* ==================== 测量更新函数（在Gui_task中调用）==================== */
void HeartRate_Update(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    /* 检查测量时间（模拟5秒测量） */
    TickType_t elapsed = xTaskGetTickCount() - measure_start_time;

    if (elapsed < pdMS_TO_TICKS(5000)) {
        /* 测量中，更新数值（模拟数据） */
        measure_counter++;
        if (measure_counter % 10 == 0) {
            hr_value = simulate_hr_reading();
            update_display();
        }
    } else {
        /* 测量完成 */
        hr_state = HR_STATE_DONE;
        hr_value = simulate_hr_reading();
        LV_LOG_USER("Heart Rate measurement done: %d BPM", hr_value);
        update_display();
    }
}

/* ==================== 模拟心率读取（实际使用时替换为MAX30102）==================== */
static uint8_t simulate_hr_reading(void)
{
    /* 模拟心率值 60-100 BPM，实际使用时从MAX30102读取 */
    static uint8_t base_hr = 72;
    int8_t variation = (measure_counter % 7) - 3;
    uint8_t hr = base_hr + variation;

    if (hr < 60) hr = 60;
    if (hr > 100) hr = 100;

    return hr;
}

/* ==================== 页面创建 ==================== */
static void page_init(void)
{
    /* 页面根对象 */
    ui_HeartRatePage = lv_obj_create(NULL);
    lv_obj_set_size(ui_HeartRatePage, 320, 240);
    lv_obj_set_style_bg_color(ui_HeartRatePage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_HeartRatePage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_HeartRatePage, LV_OBJ_FLAG_SCROLLABLE);

    /* ==================== 状态栏 ==================== */
    lv_obj_t *status_bar = lv_obj_create(ui_HeartRatePage);
    lv_obj_set_size(status_bar, 320, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, COLOR_STATUS_BAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(status_bar);
    lv_label_set_text(title, "Heart Rate");
    lv_obj_set_style_text_font(title, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* ==================== 回退按钮 ==================== */
    btn_back = lv_btn_create(ui_HeartRatePage);
    lv_obj_set_size(btn_back, BACK_BTN_SIZE, BACK_BTN_SIZE);
    lv_obj_set_pos(btn_back, 5, STATUS_BAR_HEIGHT + 5);
    lv_obj_set_style_bg_color(btn_back, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_back, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_back, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_label_create(btn_back);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_icon, FONT_BACK_BTN, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_icon, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(back_icon);

    /* ==================== 主显示区 ==================== */
    lv_obj_t *container = lv_obj_create(ui_HeartRatePage);
    lv_obj_set_size(container, 300, 160);
    lv_obj_set_pos(container, 10, STATUS_BAR_HEIGHT + 40);
    lv_obj_set_style_bg_color(container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 8, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    /* 心率图标 */
    lv_obj_t *icon = lv_label_create(container);
    lv_label_set_text(icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

    /* 心率数值显示（大字体） */
    label_hr_value = lv_label_create(container);
    lv_label_set_text(label_hr_value, "--");
    lv_obj_set_style_text_font(label_hr_value, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_hr_value, LV_ALIGN_CENTER, -10, -10);

    /* 单位 BPM */
    label_hr_unit = lv_label_create(container);
    lv_label_set_text(label_hr_unit, "BPM");
    lv_obj_set_style_text_font(label_hr_unit, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_unit, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align_to(label_hr_unit, label_hr_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 5);

    /* 状态提示 */
    label_status = lv_label_create(container);
    lv_label_set_text(label_status, "Press KEY1 to measure");
    lv_obj_set_style_text_font(label_status, FONT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -35);

    /* 按键提示 */
    label_hint = lv_label_create(container);
    lv_label_set_text(label_hint, "KEY1: Start  KEY2: Stop");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align(label_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/* ==================== 更新显示 ==================== */
static void update_display(void)
{
    if (label_hr_value == NULL || label_status == NULL) return;

    switch (hr_state) {
        case HR_STATE_IDLE:
            lv_label_set_text(label_hr_value, "--");
            lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
            lv_label_set_text(label_status, "Press KEY1 to measure");
            break;

        case HR_STATE_MEASURING:
            {
                char buf[8];
                sprintf(buf, "%d", hr_value);
                lv_label_set_text(label_hr_value, buf);
            }
            lv_obj_set_style_text_color(label_hr_value, COLOR_MENU_BTN, LV_PART_MAIN);
            lv_label_set_text(label_status, "Measuring...");
            break;

        case HR_STATE_DONE:
            {
                char buf[8];
                sprintf(buf, "%d", hr_value);
                lv_label_set_text(label_hr_value, buf);
            }
            lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_LIGHT, LV_PART_MAIN);
            lv_label_set_text(label_status, "Measurement complete");
            break;
    }
}

/* ==================== 事件回调 ==================== */
static void back_event_cb(lv_event_t *e)
{
    LV_LOG_USER("Back to HomePage");

    if (hr_state == HR_STATE_MEASURING) {
        HeartRate_StopMeasurement();
    }
    
    Key_ResetState();  // 重置按键状态

    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}
