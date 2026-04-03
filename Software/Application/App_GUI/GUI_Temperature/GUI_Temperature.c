#include "GUI_Temperature.h"
#include "key.h"
#include <string.h>
#include <stdio.h>

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_TemperaturePage = NULL;
static lv_obj_t *label_temp_value = NULL;
static lv_obj_t *label_temp_unit = NULL;
static lv_obj_t *label_status = NULL;
static lv_obj_t *label_result = NULL;
static lv_obj_t *label_hint = NULL;
static lv_obj_t *btn_back = NULL;
static lv_obj_t *bar_temp = NULL;

/* 测量状态 */
static volatile Temperature_State_t temp_state = TEMP_STATE_IDLE;
static volatile float temp_value = 0.0f;
static volatile TickType_t measure_start_time = 0;
static uint8_t measure_counter = 0;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static float simulate_temp_reading(void);
static Temperature_Result_t judge_temperature(float temp);

/* ==================== 公共函数实现 ==================== */

void GUI_Temperature_page_Init(void)
{
    if (ui_TemperaturePage != NULL) return;
    page_init();
}

void GUI_Load_TemperaturePage(void)
{
    if (ui_TemperaturePage == NULL) {
        GUI_Temperature_page_Init();
    }

    temp_state = TEMP_STATE_IDLE;
    temp_value = 0.0f;
    update_display();

    lv_disp_load_scr(ui_TemperaturePage);
}

void Temperature_StartMeasurement(void)
{
    if (temp_state == TEMP_STATE_MEASURING) return;

    temp_state = TEMP_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    measure_counter = 0;

    LV_LOG_USER("Temperature measurement started");
    update_display();
}

void Temperature_StopMeasurement(void)
{
    if (temp_state != TEMP_STATE_MEASURING) return;

    temp_state = TEMP_STATE_IDLE;
    temp_value = 0.0f;

    LV_LOG_USER("Temperature measurement stopped");
    update_display();
}

Temperature_State_t Temperature_GetState(void)
{
    return temp_state;
}

void Temperature_Update(void)
{
    if (temp_state != TEMP_STATE_MEASURING) return;

    TickType_t elapsed = xTaskGetTickCount() - measure_start_time;

    if (elapsed < pdMS_TO_TICKS(8000)) {
        measure_counter++;
        if (measure_counter % 15 == 0) {
            temp_value = simulate_temp_reading();
            update_display();
        }
    } else {
        temp_state = TEMP_STATE_DONE;
        temp_value = simulate_temp_reading();
        LV_LOG_USER("Temperature done: %.1f C", temp_value);
        update_display();
    }
}

/* ==================== 模拟读取 ==================== */
static float simulate_temp_reading(void)
{
    static float base_temp = 36.5f;
    float variation = ((measure_counter % 20) - 10) * 0.05f;
    float temp = base_temp + variation;

    if (temp < 35.0f) temp = 35.0f;
    if (temp > 42.0f) temp = 42.0f;

    return temp;
}

/* ==================== 判断结果 ==================== */
static Temperature_Result_t judge_temperature(float temp)
{
    if (temp < 36.0f) return TEMP_LOW;
    else if (temp <= 37.3f) return TEMP_NORMAL;
    else if (temp <= 38.0f) return TEMP_LOW_FEVER;
    else return TEMP_HIGH_FEVER;
}

/* ==================== 页面创建 ==================== */
static void page_init(void)
{
    ui_TemperaturePage = lv_obj_create(NULL);
    lv_obj_set_size(ui_TemperaturePage, 320, 240);
    lv_obj_set_style_bg_color(ui_TemperaturePage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_TemperaturePage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_TemperaturePage, LV_OBJ_FLAG_SCROLLABLE);

    /* 状态栏 */
    lv_obj_t *status_bar = lv_obj_create(ui_TemperaturePage);
    lv_obj_set_size(status_bar, 320, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, COLOR_STATUS_BAR, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(status_bar);
    lv_label_set_text(title, "Temperature");
    lv_obj_set_style_text_font(title, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* 回退按钮 */
    btn_back = lv_btn_create(ui_TemperaturePage);
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

    /* 主显示区 */
    lv_obj_t *container = lv_obj_create(ui_TemperaturePage);
    lv_obj_set_size(container, 300, 160);
    lv_obj_set_pos(container, 10, STATUS_BAR_HEIGHT + 40);
    lv_obj_set_style_bg_color(container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 8, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    /* TEMP标签 */
    lv_obj_t *icon = lv_label_create(container);
    lv_label_set_text(icon, "TEMP");
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 15, 10);

    /* 温度数值 */
    label_temp_value = lv_label_create(container);
    lv_label_set_text(label_temp_value, "--.");
    lv_obj_set_style_text_font(label_temp_value, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_temp_value, LV_ALIGN_RIGHT_MID, -40, -20);

    /* 单位 */
    label_temp_unit = lv_label_create(container);
    lv_label_set_text(label_temp_unit, "C");
    lv_obj_set_style_text_font(label_temp_unit, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_unit, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align_to(label_temp_unit, label_temp_value, LV_ALIGN_OUT_RIGHT_TOP, 5, 5);

    /* 温度条 */
    bar_temp = lv_bar_create(container);
    lv_obj_set_size(bar_temp, 270, 8);
    lv_obj_align(bar_temp, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_bar_set_range(bar_temp, 350, 420);
    lv_bar_set_value(bar_temp, 365, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_temp, lv_color_hex(0x2a3a5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_temp, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_temp, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_temp, 4, LV_PART_INDICATOR);

    /* 状态提示 */
    label_status = lv_label_create(container);
    lv_label_set_text(label_status, "Press KEY1 to measure");
    lv_obj_set_style_text_font(label_status, FONT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    /* 判断结果 */
    label_result = lv_label_create(container);
    lv_label_set_text(label_result, "");
    lv_obj_set_style_text_font(label_result, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_result, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label_result, LV_ALIGN_BOTTOM_MID, 0, -5);

    /* 按键提示 */
    label_hint = lv_label_create(container);
    lv_label_set_text(label_hint, "KEY1: Start  KEY2: Stop");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align(label_hint, LV_ALIGN_BOTTOM_MID, 0, 5);
}

/* ==================== 更新显示 ==================== */
static void update_display(void)
{
    if (label_temp_value == NULL || label_status == NULL) return;

    switch (temp_state) {
        case TEMP_STATE_IDLE:
            lv_label_set_text(label_temp_value, "--.");
            lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
            lv_label_set_text(label_status, "Press KEY1 to measure");
            lv_label_set_text(label_result, "");
            lv_bar_set_value(bar_temp, 365, LV_ANIM_OFF);
            break;

        case TEMP_STATE_MEASURING:
            {
                char buf[8];
                sprintf(buf, "%.1f", temp_value);
                lv_label_set_text(label_temp_value, buf);
            }
            lv_obj_set_style_text_color(label_temp_value, COLOR_ACCENT, LV_PART_MAIN);
            lv_label_set_text(label_status, "Measuring...");
            lv_label_set_text(label_result, "Please wait...");
            {
                int16_t bar_val = (int16_t)(temp_value * 10);
                lv_bar_set_value(bar_temp, bar_val, LV_ANIM_OFF);
            }
            break;

        case TEMP_STATE_DONE:
            {
                char buf[8];
                sprintf(buf, "%.1f", temp_value);
                lv_label_set_text(label_temp_value, buf);
            }
            lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_LIGHT, LV_PART_MAIN);
            lv_label_set_text(label_status, "Measurement complete");

            Temperature_Result_t result = judge_temperature(temp_value);
            switch (result) {
                case TEMP_LOW:
                    lv_label_set_text(label_result, "Low Temperature");
                    lv_obj_set_style_text_color(label_result, lv_color_hex(0x5c8aff), LV_PART_MAIN);
                    break;
                case TEMP_NORMAL:
                    lv_label_set_text(label_result, "Normal");
                    lv_obj_set_style_text_color(label_result, lv_color_hex(0x5c8a5c), LV_PART_MAIN);
                    break;
                case TEMP_LOW_FEVER:
                    lv_label_set_text(label_result, "Low Fever");
                    lv_obj_set_style_text_color(label_result, lv_color_hex(0xffaa00), LV_PART_MAIN);
                    break;
                case TEMP_HIGH_FEVER:
                    lv_label_set_text(label_result, "High Fever!");
                    lv_obj_set_style_text_color(label_result, COLOR_MENU_BTN, LV_PART_MAIN);
                    break;
            }
            {
                int16_t final_bar = (int16_t)(temp_value * 10);
                lv_bar_set_value(bar_temp, final_bar, LV_ANIM_OFF);
            }
            break;
    }
}

/* ==================== 回调 ==================== */
static void back_event_cb(lv_event_t *e)
{
    LV_LOG_USER("Back to HomePage");

    if (temp_state == TEMP_STATE_MEASURING) {
        Temperature_StopMeasurement();
    }
    
    Key_ResetState();  // 重置按键状态

    extern void GUI_Load_HomePage(void);
    
    GUI_Load_HomePage();
}
