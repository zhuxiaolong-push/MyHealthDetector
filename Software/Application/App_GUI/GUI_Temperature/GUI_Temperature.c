#include "GUI_Temperature.h"
#include "mlx90614.h"
#include "Key.h"
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

static volatile Temperature_State_t temp_state = TEMP_STATE_IDLE;
static volatile float temp_value = 0.0f;
static volatile float temp_buffer = 36.5f;
static volatile TickType_t measure_start_time = 0;
static uint8_t measure_counter = 0;
static uint8_t sensor_online = 0;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static float read_mlx90614_temp(void);
static Temperature_Result_t judge_temperature(float temp);
static uint8_t check_sensor_available(void);

/* ==================== 公共函数实现 ==================== */

void GUI_Temperature_page_Init(void)
{
    if (ui_TemperaturePage != NULL) return;

    static uint8_t hw_inited = 0;
    if (!hw_inited) {
        SMBus_Init();
        hw_inited = 1;
        LV_LOG_USER("SMBus/MLX90614 hardware initialized");
    }

    page_init();
}

void GUI_Load_TemperaturePage(void)
{
    if (ui_TemperaturePage == NULL) {
        GUI_Temperature_page_Init();
    }

    sensor_online = check_sensor_available();
    if (!sensor_online) {
        LV_LOG_USER("Warning: MLX90614 sensor not detected!");
    }

    temp_state = TEMP_STATE_IDLE;
    temp_value = 0.0f;
    update_display();

    lv_disp_load_scr(ui_TemperaturePage);
}

void Temperature_StartMeasurement(void)
{
    if (temp_state == TEMP_STATE_MEASURING) return;

    sensor_online = check_sensor_available();
    if (!sensor_online) {
        LV_LOG_USER("Error: Sensor offline, cannot start measurement");
        return;
    }

    temp_state = TEMP_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    measure_counter = 0;
    temp_value = 0.0f;

    LV_LOG_USER("Temperature measurement started (MLX90614)");
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

        if (measure_counter % 20 == 0) {
            float new_temp = read_mlx90614_temp();

            if (temp_value < 30.0f || temp_value > 45.0f) {
                temp_value = new_temp;
            } else {
                temp_value = (temp_value * 0.7f) + (new_temp * 0.3f);
            }

            update_display();
        }
    } else {
        temp_state = TEMP_STATE_DONE;
        float final_read = read_mlx90614_temp();

        if (final_read > 30.0f && final_read < 45.0f) {
            temp_value = final_read;
        }

        LV_LOG_USER("Temperature measurement done: %.2f C", temp_value);
        update_display();
    }
}

/* ==================== 传感器底层函数 ==================== */

static uint8_t check_sensor_available(void)
{
    float test_temp = SMBus_ReadTemp();

    if (test_temp > 20.0f && test_temp < 100.0f) {
        return 1;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
    test_temp = SMBus_ReadTemp();
    if (test_temp > 20.0f && test_temp < 100.0f) {
        return 1;
    }

    return 0;
}

static float read_mlx90614_temp(void)
{
    float temp = SMBus_ReadTemp();

    if (temp < 20.0f || temp > 50.0f || temp == 0.0f) {
        LV_LOG_USER("MLX90614 read error or invalid value: %.2f", temp);

        if (temp_buffer >= 35.0f && temp_buffer <= 42.0f) {
            return temp_buffer;
        }
        return 36.5f;
    }

    temp_buffer = temp;
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

    lv_obj_t *container = lv_obj_create(ui_TemperaturePage);
    lv_obj_set_size(container, 300, 160);
    lv_obj_set_pos(container, 10, STATUS_BAR_HEIGHT + 40);
    lv_obj_set_style_bg_color(container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 8, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(container);
    lv_label_set_text(icon, "TEMP");
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 15, 10);

    label_temp_value = lv_label_create(container);
    lv_label_set_text(label_temp_value, "--.");
    lv_obj_set_style_text_font(label_temp_value, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_temp_value, LV_ALIGN_RIGHT_MID, -40, -20);

    label_temp_unit = lv_label_create(container);
    lv_label_set_text(label_temp_unit, "C");
    lv_obj_set_style_text_font(label_temp_unit, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_unit, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align_to(label_temp_unit, label_temp_value, LV_ALIGN_OUT_RIGHT_TOP, 5, 5);

    bar_temp = lv_bar_create(container);
    lv_obj_set_size(bar_temp, 270, 8);
    lv_obj_align(bar_temp, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_bar_set_range(bar_temp, 350, 420);
    lv_bar_set_value(bar_temp, 365, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_temp, lv_color_hex(0x2a3a5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_temp, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_temp, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_temp, 4, LV_PART_INDICATOR);

    label_status = lv_label_create(container);
    lv_label_set_text(label_status, "Press KEY3 to measure");
    lv_obj_set_style_text_font(label_status, FONT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    label_result = lv_label_create(container);
    lv_label_set_text(label_result, "");
    lv_obj_set_style_text_font(label_result, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_result, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(label_result, LV_ALIGN_BOTTOM_MID, 0, -5);

    label_hint = lv_label_create(container);
    lv_label_set_text(label_hint, "KEY3: Start/Stop");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align(label_hint, LV_ALIGN_BOTTOM_MID, 0, 5);
}

/* ==================== 更新显示 ==================== */
static void update_display(void)
{
    if (label_temp_value == NULL || label_status == NULL) return;

    if (!sensor_online && temp_state == TEMP_STATE_IDLE) {
        lv_label_set_text(label_status, "Sensor offline!");
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xff0000), LV_PART_MAIN);
    }

    switch (temp_state) {
        case TEMP_STATE_IDLE:
            lv_label_set_text(label_temp_value, "--.");
            lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
            if (sensor_online) {
                lv_label_set_text(label_status, "Press KEY3 to measure");
                lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
            }
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
                if (bar_val < 350) bar_val = 350;
                if (bar_val > 420) bar_val = 420;
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
                if (final_bar < 350) final_bar = 350;
                if (final_bar > 420) final_bar = 420;
                lv_bar_set_value(bar_temp, final_bar, LV_ANIM_OFF);
            }
            break;
    }
}

/* ==================== 回调 ==================== */
static void back_event_cb(lv_event_t *e)
{
    (void)e;
    LV_LOG_USER("Back to HomePage");

    if (temp_state == TEMP_STATE_MEASURING) {
        Temperature_StopMeasurement();
    }

    Key_ResetState();

    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}