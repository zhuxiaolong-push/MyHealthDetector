#include "GUI_main.h"
#include <string.h>
#include "Key.h"

#if LV_COLOR_DEPTH != 16 
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

/* 页面加载函数指针类型 */
typedef void (*page_load_func_t)(void);

/* ==================== 静态变量 ==================== */
static lv_obj_t *ui_HomePage = NULL;
static lv_obj_t *status_bar = NULL;
static lv_obj_t *time_label = NULL;

/* ==================== 菜单项结构 ==================== */
typedef struct {
    const char *text;
    lv_event_cb_t event_cb;
    page_load_func_t page_init;
} menu_item_t;

/* ==================== 前向声明 ==================== */
static void status_bar_init(lv_obj_t *parent);
static void menu_panel_init(lv_obj_t *parent);
static void info_panel_init(lv_obj_t *parent);

/* ==================== 菜单按钮事件回调 ==================== */
static void on_heart_rate_click(lv_event_t *e) {
    LV_LOG_USER("Navigate to: Heart Rate Measurement");
    extern void App_SetPage(uint8_t page);
    App_SetPage(KEY_PAGE_HEART_RATE);
    GUI_Load_HeartRatePage();

}

static void on_temperature_click(lv_event_t *e) {
    LV_LOG_USER("Navigate to: Temperature Measurement");
    extern void App_SetPage(uint8_t page);
    App_SetPage(KEY_PAGE_TEMPERATURE);
    GUI_Load_TemperaturePage();
}

static void on_about_click(lv_event_t *e) {
    LV_LOG_USER("Navigate to: About Device");
    GUI_Load_AboutPage();
}

static void on_settings_click(lv_event_t *e) {
    LV_LOG_USER("Navigate to: Settings");
    GUI_Load_SettingsPage();
}

/* ==================== 公共函数实现 ==================== */
void GUI_Init(void) {
    lv_disp_t *dispp = lv_disp_get_default();
    if (dispp == NULL) {
        LV_LOG_ERROR("No display registered before GUI_Init!");
        return;
    }

    lv_theme_t *theme = lv_theme_default_init(
        dispp,
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_BLUE),
        true,
        LV_FONT_DEFAULT
    );
    lv_disp_set_theme(dispp, theme);

    LV_LOG_USER("Initializing GUI pages...");
    
    GUI_Main_page_Init();
    GUI_About_page_Init();
    GUI_Power_page_Init();
    GUI_Settings_page_Init();
    GUI_HeartRate_page_Init();
    GUI_Temperature_page_Init();

    lv_disp_load_scr(ui_HomePage);
    
    LV_LOG_USER("GUI initialized successfully, HomePage loaded");
}

void GUI_Main_page_Init(void)
{
    LV_LOG_USER("Creating HomePage (320x240 landscape)");
    
    ui_HomePage = lv_obj_create(NULL);
    lv_obj_set_size(ui_HomePage, 320, 240);
    lv_obj_set_style_bg_color(ui_HomePage, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_HomePage, 0, LV_PART_MAIN);
    lv_obj_clear_flag(ui_HomePage, LV_OBJ_FLAG_SCROLLABLE);

    status_bar_init(ui_HomePage);
    menu_panel_init(ui_HomePage);
    info_panel_init(ui_HomePage);
    
    LV_LOG_USER("HomePage created successfully");
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

    /* 左侧：Health detector v1.0 */
    lv_obj_t *title_label = lv_label_create(status_bar);
    lv_label_set_text(title_label, "Health detector v1.0");
    lv_obj_set_style_text_font(title_label, FONT_STATUS, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 5, 0);

    /* 右侧：时间 */
    time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "10:30");
    lv_obj_set_style_text_font(time_label, FONT_STATUS, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -5, 0);
}

/* ==================== 菜单栏实现 ==================== */
static void menu_panel_init(lv_obj_t *parent)
{
    LV_LOG_USER("Initializing menu panel");
    
    /* 计算面板高度 */
    #define PANEL_HEIGHT (240 - STATUS_BAR_HEIGHT)
    
    /* 菜单面板容器（左侧）- 与信息面板同色 */
    lv_obj_t *menu_panel = lv_obj_create(parent);
    lv_obj_set_size(menu_panel, MENU_WIDTH, PANEL_HEIGHT);
    lv_obj_set_pos(menu_panel, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(menu_panel, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menu_panel, PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_top(menu_panel, PADDING, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(menu_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(menu_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* 按钮布局 - 紧凑排列 */
    #define BTN_HEIGHT      32
    #define BTN_SPACING     6
    
    const menu_item_t menu_items[MENU_COUNT] = {
        {"Heart Rate",    on_heart_rate_click,    GUI_HeartRate_page_Init},
        {"Temperature",   on_temperature_click,   GUI_Temperature_page_Init},
        {"About Device",  on_about_click,         GUI_About_page_Init},
        {"Settings",      on_settings_click,      GUI_Settings_page_Init},
    };

    LV_LOG_USER("Creating %d menu buttons", (int)MENU_COUNT);

    /* 创建菜单按钮 - 紧凑排列 */
    for (int i = 0; i < MENU_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(menu_panel);
        lv_obj_set_size(btn, MENU_WIDTH - 2 * PADDING, BTN_HEIGHT);
        lv_obj_set_style_bg_color(btn, COLOR_MENU_BTN, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, COLOR_MENU_BTN_PR, LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        
        /* 紧凑垂直排列，从顶部开始 */
        lv_obj_set_pos(btn, PADDING, PADDING + i * (BTN_HEIGHT + BTN_SPACING));
        
        /* 按钮文字 */
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, menu_items[i].text);
        lv_obj_set_style_text_font(label, FONT_MENU, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
        lv_obj_center(label);
        
        /* 绑定事件 */
        lv_obj_add_event_cb(btn, menu_items[i].event_cb, LV_EVENT_CLICKED, menu_items[i].page_init);
        
        LV_LOG_USER("Menu button %d created: %s", i + 1, menu_items[i].text);
    }
}

/* ==================== 信息面板实现 ==================== */
static void info_panel_init(lv_obj_t *parent)
{
    LV_LOG_USER("Initializing info panel");
    
    #define INFO_WIDTH (320 - MENU_WIDTH)
    #define INFO_HEIGHT (240 - STATUS_BAR_HEIGHT)
    
    /* 信息面板 - 与菜单面板同色背景 */
    lv_obj_t *info_panel = lv_obj_create(parent);
    lv_obj_set_size(info_panel, INFO_WIDTH, INFO_HEIGHT);
    lv_obj_set_pos(info_panel, MENU_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(info_panel, COLOR_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_panel, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(info_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(info_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(info_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t *title = lv_label_create(info_panel);
    lv_label_set_text(title, "Instructions");
    lv_obj_set_style_text_font(title, FONT_INFO_TITLE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* 分隔线 */
    lv_obj_t *line = lv_line_create(info_panel);
    static lv_point_t line_points[] = {{0, 0}, {140, 0}};
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, COLOR_MENU_BTN, LV_PART_MAIN);
    lv_obj_set_style_line_width(line, 1, LV_PART_MAIN);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 30);

    /* 说明文字 */
    lv_obj_t *instr_label = lv_label_create(info_panel);
    lv_label_set_text(instr_label, "Click on the menu on the left to enter the function interface");
    lv_obj_set_style_text_font(instr_label, FONT_INFO_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_color(instr_label, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(instr_label, 4, LV_PART_MAIN);
    lv_label_set_long_mode(instr_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(instr_label, INFO_WIDTH - 24);
    lv_obj_align(instr_label, LV_ALIGN_CENTER, 0, 10);
    
    LV_LOG_USER("Info panel initialized with instructions");
}

/* ==================== 页面跳转接口 ==================== */
void GUI_Load_HomePage(void)
{
    LV_LOG_USER("Loading HomePage");
    lv_disp_load_scr(ui_HomePage);
}
