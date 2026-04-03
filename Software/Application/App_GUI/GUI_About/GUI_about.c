#include "GUI_About.h"
#include <string.h>

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_AboutPage = NULL;
static lv_obj_t *status_bar = NULL;
static lv_obj_t *back_btn = NULL;
static lv_obj_t *info_container = NULL;

/* ==================== 设备信息结构 ==================== */
typedef struct {
    const char *label;
    const char *value;
} device_info_t;

/* ==================== 前向声明 ==================== */
static void status_bar_init(lv_obj_t *parent);
static void back_button_init(lv_obj_t *parent);
static void info_panel_init(lv_obj_t *parent);
static void on_back_click(lv_event_t *e);

/* ==================== 设备信息数据 ==================== */
static const device_info_t device_info[] = {
    {"Main Controller",     "STM32F407VET6"},
    {"GUI Framework",       "LVGL"},
    {"Operating System",    "FreeRTOS"},
    {"Designer",            "zhuxiaolong"},
};
#define INFO_COUNT (sizeof(device_info) / sizeof(device_info[0]))

/* ==================== 事件回调 ==================== */
static void on_back_click(lv_event_t *e) {
    LV_LOG_USER("Navigate back to: HomePage");
    extern void GUI_Load_HomePage(void);
    GUI_Load_HomePage();
}

/* ==================== 公共函数实现 ==================== */
void GUI_About_page_Init(void)
{
    LV_LOG_USER("Creating AboutPage (320x240 landscape)");

    /* 如果页面已存在，先删除 */
    if (ui_AboutPage != NULL) {
        lv_obj_del(ui_AboutPage);
    }

    ui_AboutPage = lv_obj_create(NULL);
    lv_obj_set_size(ui_AboutPage, 320, 240);
    lv_obj_set_style_bg_color(ui_AboutPage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_AboutPage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_AboutPage, LV_OBJ_FLAG_SCROLLABLE);

    status_bar_init(ui_AboutPage);
    back_button_init(ui_AboutPage);
    info_panel_init(ui_AboutPage);

    LV_LOG_USER("AboutPage created successfully");
}

void GUI_Load_AboutPage(void)
{
    LV_LOG_USER("Loading AboutPage");
    if (ui_AboutPage == NULL) {
        GUI_About_page_Init();
    }
    lv_disp_load_scr(ui_AboutPage);
}

/* ==================== 状态栏实现 ==================== */
static void status_bar_init(lv_obj_t *parent)
{
    LV_LOG_USER("Initializing status bar");

    status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar, 320, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, COLOR_STATUS_BAR, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 居中标题 */
    lv_obj_t *title_label = lv_label_create(status_bar);
    lv_label_set_text(title_label, "About Device");
    lv_obj_set_style_text_font(title_label, FONT_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
}

/* ==================== 回退按钮实现 ==================== */
static void back_button_init(lv_obj_t *parent)
{
    LV_LOG_USER("Initializing back button");

    /* 回退按钮 - 左上角 */
    back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, BACK_BTN_SIZE, BACK_BTN_SIZE);
    lv_obj_set_pos(back_btn, 5, STATUS_BAR_HEIGHT + 5);
    lv_obj_set_style_bg_color(back_btn, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back_btn, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);

    /* 按钮图标 - 使用左箭头符号 */
    lv_obj_t *btn_label = lv_label_create(back_btn);
    lv_label_set_text(btn_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(btn_label, FONT_BACK_BTN, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_center(btn_label);

    /* 绑定点击事件 */
    lv_obj_add_event_cb(back_btn, on_back_click, LV_EVENT_CLICKED, NULL);
}

/* ==================== 信息面板实现 ==================== */
static void info_panel_init(lv_obj_t *parent)
{
    LV_LOG_USER("Initializing info panel");

    #define PANEL_Y_START   (STATUS_BAR_HEIGHT + BACK_BTN_SIZE + 15)
    #define PANEL_HEIGHT    (240 - PANEL_Y_START - 5)
    #define PANEL_WIDTH     300
    #define LABEL_WIDTH     110     /* 标签固定宽度 */
    #define VALUE_X_OFFSET  135     /* 值的X偏移 */

    /* 信息面板容器 */
    info_container = lv_obj_create(parent);
    lv_obj_set_size(info_container, PANEL_WIDTH, PANEL_HEIGHT);
    lv_obj_set_pos(info_container, 10, PANEL_Y_START);
    lv_obj_set_style_bg_color(info_container, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_container, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(info_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(info_container, 6, LV_PART_MAIN);
    lv_obj_clear_flag(info_container, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建设备信息列表 - 标签和值在同一行 */
    for (int i = 0; i < INFO_COUNT; i++) {
        /* 信息项容器 */
        lv_obj_t *item_container = lv_obj_create(info_container);
        lv_obj_set_size(item_container, PANEL_WIDTH - 30, INFO_ITEM_HEIGHT);
        lv_obj_set_style_bg_color(item_container, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
        lv_obj_set_style_radius(item_container, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(item_container, 0, LV_PART_MAIN);
        lv_obj_clear_flag(item_container, LV_OBJ_FLAG_SCROLLABLE);

        /* 垂直排列 */
        lv_obj_set_pos(item_container, 0, i * (INFO_ITEM_HEIGHT + INFO_ITEM_SPACING));

        /* 标签（左侧）- 灰色小字 */
        lv_obj_t *label_text = lv_label_create(item_container);
        lv_label_set_text(label_text, device_info[i].label);
        lv_obj_set_style_text_font(label_text, FONT_INFO_LABEL, LV_PART_MAIN);
        lv_obj_set_style_text_color(label_text, COLOR_TEXT_GRAY, LV_PART_MAIN);
        lv_obj_set_width(label_text, LABEL_WIDTH);
        lv_obj_align(label_text, LV_ALIGN_LEFT_MID, 10, 0);

        /* 值（右侧）- 白色正常字，与标签同一行 */
        lv_obj_t *value_text = lv_label_create(item_container);
        lv_label_set_text(value_text, device_info[i].value);
        lv_obj_set_style_text_font(value_text, FONT_INFO_VALUE, LV_PART_MAIN);
        lv_obj_set_style_text_color(value_text, COLOR_TEXT_LIGHT, LV_PART_MAIN);
        lv_obj_align(value_text, LV_ALIGN_LEFT_MID, VALUE_X_OFFSET, 0);

        LV_LOG_USER("Info item %d created: %s = %s", i + 1, 
                    device_info[i].label, device_info[i].value);
    }

    LV_LOG_USER("Info panel initialized with %d items", (int)INFO_COUNT);
}
