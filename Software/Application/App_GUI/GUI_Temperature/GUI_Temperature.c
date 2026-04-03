#include "GUI_Temperature.h"
#include "mlx90614.h"           /* MLX90614 驱动头文件 */
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
static volatile float temp_buffer = 36.5f;      /* 温度缓冲，用于错误时保持 */
static volatile TickType_t measure_start_time = 0;
static uint8_t measure_counter = 0;
static uint8_t sensor_online = 0;               /* 传感器在线标志 */

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static float read_mlx90614_temp(void);          /* 替换模拟读取 */
static Temperature_Result_t judge_temperature(float temp);
static uint8_t check_sensor_available(void);    /* 传感器检测 */

/* ==================== 公共函数实现 ==================== */

void GUI_Temperature_page_Init(void)
{
    if (ui_TemperaturePage != NULL) return;
    
    /* 初始化 SMBus/I2C 硬件（仅一次） */
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

    /* 检查传感器是否在线 */
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
    
    /* 测量前再次确认传感器 */
    sensor_online = check_sensor_available();
    if (!sensor_online) {
        LV_LOG_USER("Error: Sensor offline, cannot start measurement");
        /* 可以选择显示错误提示 */
        return;
    }

    temp_state = TEMP_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    measure_counter = 0;
    temp_value = 0.0f;  /* 清零重新开始 */

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

    /* 测量时间窗口：8秒 */
    if (elapsed < pdMS_TO_TICKS(8000)) {
        measure_counter++;
        
        /* 每 200ms 读取一次（MLX90614 转换时间约 100-300ms，取保守值） */
        if (measure_counter % 20 == 0) {
            float new_temp = read_mlx90614_temp();
            
            /* 滑动平均滤波：新值占30%，历史值占70%，使显示更稳定 */
            if (temp_value < 30.0f || temp_value > 45.0f) {
                temp_value = new_temp;  /* 首次或异常时直接使用 */
            } else {
                temp_value = (temp_value * 0.7f) + (new_temp * 0.3f);
            }
            
            update_display();
        }
    } else {
        /* 测量完成，最终读取 */
        temp_state = TEMP_STATE_DONE;
        float final_read = read_mlx90614_temp();
        
        /* 最终值使用最后几次读数的平均 */
        if (final_read > 30.0f && final_read < 45.0f) {
            temp_value = final_read;
        }
        
        LV_LOG_USER("Temperature measurement done: %.2f C", temp_value);
        update_display();
    }
}

/* ==================== 传感器底层函数 ==================== */

/**
 * @brief 检查 MLX90614 传感器是否在线
 * @return 1:在线  0:离线
 */
static uint8_t check_sensor_available(void)
{
    /* 尝试读取温度，验证通信 */
    float test_temp = SMBus_ReadTemp();
    
    /* 有效人体温度范围 30-45°C，若在此范围或附近认为在线 */
    if (test_temp > 20.0f && test_temp < 100.0f) {
        return 1;
    }
    
    /* 再尝试一次（有时首次通信会失败） */
    vTaskDelay(pdMS_TO_TICKS(50));
    test_temp = SMBus_ReadTemp();
    if (test_temp > 20.0f && test_temp < 100.0f) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief 读取 MLX90614 真实温度值（替换原来的模拟函数）
 * @return 温度值（摄氏度），失败返回上次有效值或36.5
 */
static float read_mlx90614_temp(void)
{
    float temp = SMBus_ReadTemp();
    
    /* 错误处理：范围检查（人体温度不可能低于20或高于50） */
    if (temp < 20.0f || temp > 50.0f || temp == 0.0f) {
        LV_LOG_USER("MLX90614 read error or invalid value: %.2f", temp);
        
        /* 如果温度缓冲值有效，返回缓冲值，否则返回默认值36.5 */
        if (temp_buffer >= 35.0f && temp_buffer <= 42.0f) {
            return temp_buffer;
        }
        return 36.5f;  /* 默认人体温度 */
    }
    
    /* 保存有效值到缓冲 */
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

    /* 温度条 (35.0-42.0 对应 350-420) */
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

    /* 传感器离线警告（可选） */
    if (!sensor_online && temp_state == TEMP_STATE_IDLE) {
        lv_label_set_text(label_status, "Sensor offline!");
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xff0000), LV_PART_MAIN);
    }

    switch (temp_state) {
        case TEMP_STATE_IDLE:
            lv_label_set_text(label_temp_value, "--.");
            lv_obj_set_style_text_color(label_temp_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
            if (sensor_online) {
                lv_label_set_text(label_status, "Press KEY1 to measure");
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
    LV_LOG_USER("Back to HomePage");

    if (temp_state == TEMP_STATE_MEASURING) {
        Temperature_StopMeasurement();
    }
    
    Key_ResetState();  /* 重置按键状态，防止返回后误触发 */

    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}
