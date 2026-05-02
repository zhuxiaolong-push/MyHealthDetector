#include "GUI_HeartRate.h"
#include "Key.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ==================== 模拟心率配置 ==================== */
#define HR_BASELINE       72   /* 典型静息心率基准值 */
#define HR_VARIATION      7    /* 每次测量偏移范围 ±7，结果落在 65~79 */
#define HR_FLUCTUATION    1    /* 同一次测量内最大波动 ±1 */

/* ==================== 配置参数 ==================== */
#define HR_MIN_VALID      30
#define HR_MAX_VALID      220
#define HR_NORMAL_MIN     60
#define HR_NORMAL_MAX     100
#define HR_WARNING_LOW    40
#define HR_WARNING_HIGH   180

/* ==================== 静态UI对象 ==================== */
static lv_obj_t *ui_HeartRatePage = NULL;
static lv_obj_t *label_hr_value = NULL;
static lv_obj_t *label_hr_unit = NULL;
static lv_obj_t *label_status = NULL;
static lv_obj_t *label_hint = NULL;
static lv_obj_t *btn_back = NULL;
static lv_obj_t *bar_progress = NULL;
static lv_obj_t *icon_heart = NULL;

/* ==================== 测量状态变量 ==================== */
static volatile HeartRate_State_t hr_state = HR_STATE_IDLE;
static volatile uint8_t hr_value = 0;
static volatile uint32_t measure_start_time = 0;
static volatile uint8_t hr_valid_flag = 0;

static uint16_t sample_counter = 0;
static uint8_t  rand_inited = 0;
static uint8_t  sim_target_hr = 0;   /* 本次测量选定的稳定心率 */

/* ==================== 前向声明 ==================== */
static void page_init(void);
static void back_event_cb(lv_event_t *e);
static void update_display(void);
static lv_color_t get_hr_color(uint8_t hr);

/* ==================== 公共接口实现 ==================== */

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

    hr_state = HR_STATE_IDLE;
    hr_value = 0;
    sample_counter = 0;
    hr_valid_flag = 0;
    sim_target_hr = 0;

    update_display();

    lv_disp_load_scr(ui_HeartRatePage);
    Key_SetActivePage(KEY_PAGE_HEARTRATE);
}

void HeartRate_StartMeasurement(void)
{
    if (hr_state == HR_STATE_MEASURING) return;

    if (!rand_inited) {
        srand((unsigned int)xTaskGetTickCount());
        rand_inited = 1;
    }

    /* 在基准值附近小范围随机，结果集中在 65~79，符合静息心率 */
    int8_t offset = (rand() % (2 * HR_VARIATION + 1)) - HR_VARIATION; /* -7 ~ +7 */
    int16_t target = HR_BASELINE + offset;
    if (target < HR_NORMAL_MIN) target = HR_NORMAL_MIN;
    if (target > HR_NORMAL_MAX) target = HR_NORMAL_MAX;
    sim_target_hr = (uint8_t)target;

    hr_state = HR_STATE_MEASURING;
    measure_start_time = xTaskGetTickCount();
    sample_counter = 0;
    hr_valid_flag = 0;

    LV_LOG_USER("HR measurement started, target=%d BPM", sim_target_hr);
    update_display();
}

void HeartRate_StopMeasurement(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    hr_state = HR_STATE_IDLE;
    hr_value = 0;
    sample_counter = 0;
    sim_target_hr = 0;

    LV_LOG_USER("HR measurement stopped");
    update_display();
}

HeartRate_State_t HeartRate_GetState(void)
{
    return hr_state;
}

uint8_t HeartRate_GetValue(void)
{
    return hr_valid_flag ? hr_value : 0;
}

/* ==================== 模拟数据采集 ==================== */

void HeartRate_Update(void)
{
    if (hr_state != HR_STATE_MEASURING) return;

    if (sample_counter < BUFFER_SIZE) {
        sample_counter++;

        /* 每 50 个样本更新一次进度和显示 */
        if (sample_counter % 50 == 0) {
            uint8_t progress = (sample_counter * 100) / BUFFER_SIZE;

            /* 同一次测量只 ±1 波动，视觉上几乎稳定 */
            int8_t fluctuation = (rand() % (2 * HR_FLUCTUATION + 1)) - HR_FLUCTUATION; /* -1 ~ +1 */
            int16_t display_hr = (int16_t)sim_target_hr + fluctuation;
            if (display_hr < HR_MIN_VALID) display_hr = HR_MIN_VALID;
            if (display_hr > HR_MAX_VALID) display_hr = HR_MAX_VALID;

            if (label_hr_value && bar_progress) {
                char buf[8];
                sprintf(buf, "%d", display_hr);
                lv_label_set_text(label_hr_value, buf);
                lv_bar_set_value(bar_progress, progress, LV_ANIM_OFF);
            }
        }

        /* 6 秒超时保护 */
        if ((xTaskGetTickCount() - measure_start_time) > pdMS_TO_TICKS(6000)) {
            LV_LOG_USER("HR timeout");
            hr_state = HR_STATE_DONE;
            hr_value = 0;
            hr_valid_flag = 0;
            update_display();
        }
    }
    else {
        /* 采样完成，使用本次选定的稳定值作为结果 */
        hr_state = HR_STATE_DONE;

        if (sim_target_hr > HR_MIN_VALID && sim_target_hr < HR_MAX_VALID) {
            hr_value = sim_target_hr;
            hr_valid_flag = 1;
            LV_LOG_USER("HR done: %d BPM", hr_value);
        } else {
            hr_value = 0;
            hr_valid_flag = 0;
        }

        update_display();
    }
}

/* ==================== UI辅助函数 ==================== */

static lv_color_t get_hr_color(uint8_t hr)
{
    if (hr >= HR_NORMAL_MIN && hr <= HR_NORMAL_MAX) {
        return lv_color_hex(0x00ff00);
    } else if ((hr >= HR_WARNING_LOW && hr < HR_NORMAL_MIN) ||
               (hr > HR_NORMAL_MAX && hr <= HR_WARNING_HIGH)) {
        return lv_color_hex(0xffaa00);
    } else {
        return lv_color_hex(0xff0000);
    }
}

/* ==================== UI创建 ==================== */

static void page_init(void)
{
    ui_HeartRatePage = lv_obj_create(NULL);
    lv_obj_set_size(ui_HeartRatePage, 320, 240);
    lv_obj_set_style_bg_color(ui_HeartRatePage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_HeartRatePage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_HeartRatePage, LV_OBJ_FLAG_SCROLLABLE);

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

    btn_back = lv_btn_create(ui_HeartRatePage);
    lv_obj_set_size(btn_back, BACK_BTN_SIZE, BACK_BTN_SIZE);
    lv_obj_set_pos(btn_back, 5, STATUS_BAR_HEIGHT + 5);
    lv_obj_set_style_bg_color(btn_back, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_back, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_label_create(btn_back);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_icon, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(back_icon);

    lv_obj_t *container = lv_obj_create(ui_HeartRatePage);
    lv_obj_set_size(container, 300, 160);
    lv_obj_set_pos(container, 10, STATUS_BAR_HEIGHT + 40);
    lv_obj_set_style_bg_color(container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 8, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    icon_heart = lv_label_create(container);
    lv_label_set_text(icon_heart, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(icon_heart, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_heart, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_align(icon_heart, LV_ALIGN_TOP_MID, 0, 10);

    label_hr_value = lv_label_create(container);
    lv_label_set_text(label_hr_value, "--");
    lv_obj_set_style_text_font(label_hr_value, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_value, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_hr_value, LV_ALIGN_CENTER, -10, -20);

    label_hr_unit = lv_label_create(container);
    lv_label_set_text(label_hr_unit, "BPM");
    lv_obj_set_style_text_font(label_hr_unit, FONT_MENU, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hr_unit, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align_to(label_hr_unit, label_hr_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 5);

    bar_progress = lv_bar_create(container);
    lv_obj_set_size(bar_progress, 200, 6);
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, 25);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, lv_color_hex(0x2a3a5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, COLOR_MENU_BTN, LV_PART_INDICATOR);
    lv_obj_add_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);

    label_status = lv_label_create(container);
    lv_label_set_text(label_status, "Press KEY3 to measure");
    lv_obj_set_style_text_font(label_status, FONT_HINT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -30);

    label_hint = lv_label_create(container);
    lv_label_set_text(label_hint, "KEY3: Start/Stop");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align(label_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

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
            lv_label_set_text(icon_heart, LV_SYMBOL_PLAY);
            break;

        case HR_STATE_MEASURING:
            lv_obj_set_style_text_color(label_hr_value, COLOR_MENU_BTN, LV_PART_MAIN);
            lv_label_set_text(label_status, "Collecting data...");
            lv_obj_clear_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(icon_heart, LV_SYMBOL_STOP);
            break;

        case HR_STATE_DONE:
            {
                char buf[8];
                if (hr_valid_flag && hr_value > 0) {
                    sprintf(buf, "%d", hr_value);
                    lv_label_set_text(label_hr_value, buf);

                    lv_color_t hr_color = get_hr_color(hr_value);
                    lv_obj_set_style_text_color(label_hr_value, hr_color, LV_PART_MAIN);

                    if (hr_value >= HR_NORMAL_MIN && hr_value <= HR_NORMAL_MAX) {
                        lv_label_set_text(label_status, "Normal (60-100 BPM)");
                    } else if (hr_value < HR_NORMAL_MIN) {
                        lv_label_set_text(label_status, "Bradycardia detected");
                    } else {
                        lv_label_set_text(label_status, "Tachycardia detected");
                    }
                } else {
                    lv_label_set_text(label_hr_value, "--");
                    lv_obj_set_style_text_color(label_hr_value, lv_color_hex(0xff0000), LV_PART_MAIN);
                    lv_label_set_text(label_status, "Failed - Try again");
                }
            }
            lv_obj_add_flag(bar_progress, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(icon_heart, LV_SYMBOL_OK);
            break;
    }
}

static void back_event_cb(lv_event_t *e)
{
    (void)e;
    LV_LOG_USER("Back to HomePage");

    if (hr_state == HR_STATE_MEASURING) {
        HeartRate_StopMeasurement();
    }

    Key_SetActivePage(KEY_PAGE_NONE);
    Key_ResetState();

    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}