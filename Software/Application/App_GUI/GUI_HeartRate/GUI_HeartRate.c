#include "GUI_HeartRate.h"
#include "max30102.h"           // 添加 MAX30102 驱动
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
static lv_obj_t *bar_progress = NULL;     // 采集进度条

/* 测量状态 */
static volatile HeartRate_State_t hr_state = HR_STATE_IDLE;
static volatile uint8_t hr_value = 0;
static volatile uint32_t measure_start_time = 0;

/* MAX30102 数据缓冲区（根据 max30102.h 中的 BUFFER_SIZE）*/
static uint32_t ir_buffer[BUFFER_SIZE];     // BUFFER_SIZE = 500
static uint32_t red_buffer[BUFFER_SIZE];
static uint16_t sample_counter = 0;
static uint8_t hr_valid_flag = 0;

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static void reset_measurement_buffers(void);

/* ==================== 公共函数实现 ==================== */

void GUI_HeartRate_page_Init(void)
{
    if (ui_HeartRatePage != NULL) return;
    
    /* 初始化 MAX30102 硬件（仅执行一次） */
    static uint8_t hw_initialized = 0;
    if (!hw_initialized) {
        MAX30102_Init();
        hw_initialized = 1;
        LV_LOG_USER("MAX30102 hardware initialized");
    }
    
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
    sample_counter = 0;
    hr_valid_flag = 0;
    reset_measurement_buffers();
    update_display();

    lv_disp_load_scr(ui_HeartRatePage);
}

/* ==================== 测量控制接口 ==================== */

void HeartRate_StartMeasurement(void)
{
    if (hr_state == HR_STATE_MEASURING) return;

    hr_state = HR_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    sample_counter = 0;
    hr_valid_flag = 0;
    reset_measurement_buffers();

    LV_LOG_USER("Heart Rate measurement started (MAX30102)");
    update_display();
}

void HeartRate_StopMeasurement(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    hr_state = HR_STATE_IDLE;
    hr_value = 0;
    sample_counter = 0;

    LV_LOG_USER("Heart Rate measurement stopped");
    update_display();
}

HeartRate_State_t HeartRate_GetState(void)
{
    return hr_state;
}

/* ==================== MAX30102 真实数据采集 ==================== */

static void reset_measurement_buffers(void)
{
    memset(ir_buffer, 0, sizeof(ir_buffer));
    memset(red_buffer, 0, sizeof(red_buffer));
}

/**
 * @brief 心率更新函数 - 使用真实 MAX30102 数据
 * 采集 500 个样本（5秒 @ 100Hz），然后调用官方算法计算
 */
void HeartRate_Update(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    /* 采集数据阶段 */
    if (sample_counter < BUFFER_SIZE) {
        uint32_t red_led = 0;
        uint32_t ir_led = 0;
        
        /* 从 MAX30102 FIFO 读取数据 */
        maxim_max30102_read_fifo(&red_led, &ir_led);
        
        /* 存储数据（屏蔽 MSB，保留 18 位有效数据）*/
        red_buffer[sample_counter] = red_led & 0x03FFFF;
        ir_buffer[sample_counter] = ir_led & 0x03FFFF;
        sample_counter++;
        
        /* 每采集 50 个样本（0.5秒）更新一次进度显示 */
        if (sample_counter % 50 == 0) {
            uint8_t progress = (sample_counter * 100) / BUFFER_SIZE;
            
            /* 临时显示进度百分比 */
            if (label_hr_value) {
                char buf[8];
                sprintf(buf, "%d%%", progress);
                lv_label_set_text(label_hr_value, buf);
                
                /* 更新进度条 */
                if (bar_progress) {
                    lv_bar_set_value(bar_progress, progress, LV_ANIM_OFF);
                }
            }
        }
        
        /* 超时保护（6秒）*/
        TickType_t elapsed = xTaskGetTickCount() - measure_start_time;
        if (elapsed > pdMS_TO_TICKS(6000)) {
            LV_LOG_USER("HR Measurement timeout");
            hr_state = HR_STATE_DONE;
            hr_value = 0;
            update_display();
        }
    } 
    else {
        /* 缓冲区满，计算心率 */
        int32_t n_heart_rate = 0;
        int8_t ch_hr_valid = 0;
        int32_t n_spo2 = 0;
        int8_t ch_spo2_valid = 0;
        
        LV_LOG_USER("HR Buffer full (%d samples), calculating...", BUFFER_SIZE);
        
        /* 调用 MAX30102 官方算法计算心率和血氧 */
        maxim_heart_rate_and_oxygen_saturation(
            ir_buffer, 
            BUFFER_SIZE, 
            red_buffer,
            &n_spo2, 
            &ch_spo2_valid,
            &n_heart_rate, 
            &ch_hr_valid
        );
        
        hr_state = HR_STATE_DONE;
        
        if (ch_hr_valid && n_heart_rate > 30 && n_heart_rate < 220) {
            hr_value = (uint8_t)n_heart_rate;
            hr_valid_flag = 1;
            LV_LOG_USER("HR Result: %d BPM, SpO2: %d%%", hr_value, n_spo2);
        } else {
            hr_value = 0;
            hr_valid_flag = 0;
            LV_LOG_USER("HR Calculation invalid");
        }
        
        update_display();
    }
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

    /* 心率数值显示 */
    label_hr_value = lv_label_create(container);
    lv_label_set_text(label_hr_value, "--");
    lv_obj_set_style_text_font(label_hr_value, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_hr_value, LV_ALIGN_CENTER, -10, -20);

    /* 单位 BPM */
    label_hr_unit = lv_label_create(container);
    lv_label_set_text(label_hr_unit, "BPM");
    lv_obj_set_style_text_font(label_hr_unit, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_unit, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align_to(label_hr_unit, label_hr_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 5);

    /* 采集进度条（测量时显示） */
    bar_progress = lv_bar_create(container);
    lv_obj_set_size(bar_progress, 200, 6);
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, 25);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, lv_color_hex(0x2a3a5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_add_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);  // 默认隐藏

    /* 状态提示 */
    label_status = lv_label_create(container);
    lv_label_set_text(label_status, "Press KEY3 to measure");
    lv_obj_set_style_text_font(label_status, FONT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -30);

    /* 按键提示 - 改为 KEY3 */
    label_hint = lv_label_create(container);
    lv_label_set_text(label_hint, "KEY3: Start/Stop");
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
            lv_label_set_text(label_status, "Press KEY3 to measure");
            lv_obj_add_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
            break;

        case HR_STATE_MEASURING:
            /* 数值在 HeartRate_Update 中更新为进度百分比 */
            lv_obj_set_style_text_color(label_hr_value, COLOR_MENU_BTN, LV_PART_MAIN);
            lv_label_set_text(label_status, "Collecting data...");
            lv_obj_clear_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);
            break;

        case HR_STATE_DONE:
            {
                char buf[8];
                if (hr_valid_flag && hr_value > 0) {
                    sprintf(buf, "%d", hr_value);
                    lv_label_set_text(label_hr_value, buf);
                    lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_LIGHT, LV_PART_MAIN);
                    lv_label_set_text(label_status, "Measurement complete");
                } else {
                    lv_label_set_text(label_hr_value, "--");
                    lv_obj_set_style_text_color(label_hr_value, lv_color_hex(0xff0000), LV_PART_MAIN);
                    lv_label_set_text(label_status, "Failed - Try again");
                }
            }
            lv_obj_add_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);
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
    
    // 通知 App_task 重置页面状态
    extern void App_SetPage(uint8_t page);
    App_SetPage(KEY_PAGE_NONE);
    
    Key_ResetState();

    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}