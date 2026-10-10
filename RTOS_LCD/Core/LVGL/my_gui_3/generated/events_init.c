/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "cmsis_os.h"

#include "color.h"
#include "MLX90640_API.h"
#include "main.h"
#include "app_state.h"
#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

extern color_mode dis_mode;
extern float max_temp, min_temp, cent_temp;
extern volatile uint32_t g_thermal_fps_x10;
extern volatile uint32_t g_thermal_last_frame_tick;

volatile MLX90640_RefreshRate refresh_rate = MLX90640_32HZ; // 由采集任务异步应用

static lv_obj_t *battery_screen;
static lv_obj_t *battery_bar;
static lv_obj_t *battery_soc_label;
static lv_obj_t *battery_voltage_label;
static lv_obj_t *battery_status_label;
static lv_obj_t *battery_back_button;

static void battery_screen_create(void);

static void screen_btn_5_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SHORT_CLICKED:
    {
        dis_mode++;
        if(dis_mode > medical_mode)
        {
            dis_mode = colorful_mode;
        }
        if(dis_mode == colorful_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "伪彩模式");
            lv_label_set_text(guider_ui.screen_label_2, "模式:伪彩");
        }
        else if(dis_mode == white_hot_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "白热模式");
            lv_label_set_text(guider_ui.screen_label_2, "模式:白热");
        }
        else if(dis_mode == black_hot_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "黑热模式");
            lv_label_set_text(guider_ui.screen_label_2, "模式:黑热");
        }
        else if(dis_mode == medical_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "医疗模式");
            lv_label_set_text(guider_ui.screen_label_2, "模式:医疗");
        }
        break;
    }
    default:
        break;
    }
}

static void screen_btn_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SHORT_CLICKED:
    {
        refresh_rate++;
        if(refresh_rate > MLX90640_64HZ)
        {
            refresh_rate = MLX90640_4HZ;
        }
        /* 不在LVGL事件回调中访问I2C；采集任务会异步应用该设置。 */
        break;
    }
    default:
        break;
    }
}

static void screen_btn_7_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
    {
        g_app_page = APP_PAGE_BATTERY;
        lv_scr_load_anim(battery_screen, LV_SCR_LOAD_ANIM_NONE, 0U, 0U, false);
    }
}

static void battery_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
    {
        lv_scr_load_anim(guider_ui.screen, LV_SCR_LOAD_ANIM_NONE, 0U, 0U, false);
        g_app_page = APP_PAGE_THERMAL;
        lv_obj_invalidate(guider_ui.screen);
    }
}

static lv_obj_t *battery_label_create(lv_obj_t *parent, const char *text,
                                      lv_coord_t y, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_width(label, 280);
    lv_obj_set_pos(label, 20, y);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0x20242a),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    return label;
}

static void battery_screen_create(void)
{
    lv_obj_t *title;
    lv_obj_t *card;

    battery_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(battery_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(battery_screen, lv_color_hex(0xf2f5f7),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(battery_screen, LV_OPA_COVER,
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    title = battery_label_create(battery_screen,
                                 "\xE7\x94\xB5\xE6\xB1\xA0\xE7\x8A\xB6\xE6\x80\x81",
                                 10, &lv_font_SourceHanSansCN_Light_2_16);
    lv_obj_set_style_text_color(title, lv_color_hex(0x1565c0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    card = lv_obj_create(battery_screen);
    lv_obj_set_pos(card, 20, 40);
    lv_obj_set_size(card, 280, 150);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0xc5ccd3),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xffffff),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

    battery_bar = lv_bar_create(card);
    lv_obj_set_pos(battery_bar, 25, 24);
    lv_obj_set_size(battery_bar, 230, 30);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0xe0e4e8),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x35b86b),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);

    battery_soc_label = battery_label_create(card, "--.-%", 60,
                                             &lv_font_SourceHanSansCN_Light_2_16);
    lv_obj_set_pos(battery_soc_label, 0, 60);
    lv_obj_set_width(battery_soc_label, 278);

    battery_voltage_label = battery_label_create(
        card, "\xE7\x94\xB5\xE5\x8E\x8B: -.---V", 90,
        &lv_font_SourceHanSansCN_Light_2_12);
    lv_obj_set_pos(battery_voltage_label, 0, 90);
    lv_obj_set_width(battery_voltage_label, 278);

    battery_status_label = battery_label_create(
        card, "\xE7\x8A\xB6\xE6\x80\x81: \xE6\xA3\x80\xE6\xB5\x8B\xE4\xB8\xAD", 116,
        &lv_font_SourceHanSansCN_Light_2_12);
    lv_obj_set_pos(battery_status_label, 0, 116);
    lv_obj_set_width(battery_status_label, 278);

    battery_back_button = lv_btn_create(battery_screen);
    lv_obj_set_pos(battery_back_button, 90, 198);
    lv_obj_set_size(battery_back_button, 140, 36);
    lv_obj_set_style_radius(battery_back_button, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(battery_back_button, lv_color_hex(0x2195f6),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    title = lv_label_create(battery_back_button);
    lv_label_set_text(title,
                      "\xE8\xBF\x94\xE5\x9B\x9E\xE7\x83\xAD\xE5\x9B\xBE");
    lv_obj_set_style_text_font(title, &lv_font_SourceHanSansCN_Light_2_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(title);
    lv_obj_add_event_cb(battery_back_button, battery_back_event_handler,
                        LV_EVENT_ALL, NULL);
}

static uint32_t refresh_rate_to_hz(MLX90640_RefreshRate rate)
{
    switch (rate)
    {
    case MLX90640_4HZ:  return 4U;
    case MLX90640_8HZ:  return 8U;
    case MLX90640_16HZ: return 16U;
    case MLX90640_32HZ: return 32U;
    case MLX90640_64HZ: return 64U;
    default:            return 0U;
    }
}

// 更新温度显示
static void update_temperature_handler(lv_timer_t *timer)
{
    int32_t center_x100;
    int32_t maximum_x100;
    uint32_t actual_fps_x10;
    uint32_t now;

    LV_UNUSED(timer);
    if (g_app_page == APP_PAGE_BATTERY)
    {
        uint32_t battery_now = osKernelGetTickCount();
        uint8_t connected = g_battery_connected;

        if ((g_battery_last_update_tick == 0U) ||
            ((battery_now - g_battery_last_update_tick) > 3000U))
        {
            connected = 0U;
        }

        if (connected != 0U)
        {
            lv_bar_set_value(battery_bar, (int32_t)(g_battery_soc_x10 / 10U),
                             LV_ANIM_ON);
            lv_label_set_text_fmt(battery_soc_label, "%u.%u%%",
                                  g_battery_soc_x10 / 10U,
                                  g_battery_soc_x10 % 10U);
            lv_label_set_text_fmt(battery_voltage_label,
                                  "\xE7\x94\xB5\xE5\x8E\x8B: %u.%03uV",
                                  g_battery_voltage_mv / 1000U,
                                  g_battery_voltage_mv % 1000U);
            lv_label_set_text_fmt(battery_status_label,
                                  "\xE7\x8A\xB6\xE6\x80\x81: \xE6\xAD\xA3\xE5\xB8\xB8  VER:0x%02X",
                                  g_battery_version);
            lv_obj_set_style_text_color(battery_status_label, lv_color_hex(0x238636),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_bar_set_value(battery_bar, 0, LV_ANIM_OFF);
            lv_label_set_text(battery_soc_label, "--.-%");
            lv_label_set_text(battery_voltage_label,
                              "\xE7\x94\xB5\xE5\x8E\x8B: -.---V");
            lv_label_set_text_fmt(battery_status_label,
                                  "\xE7\x8A\xB6\xE6\x80\x81: \xE5\xA4\xB1\xE8\xB4\xA5  E:%d",
                                  g_battery_error);
            lv_obj_set_style_text_color(battery_status_label, lv_color_hex(0xc62828),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        return;
    }

    center_x100 = (int32_t)(cent_temp * 100.0f);
    maximum_x100 = (int32_t)(max_temp * 100.0f);
    actual_fps_x10 = g_thermal_fps_x10;
    now = osKernelGetTickCount();
    if ((g_thermal_last_frame_tick == 0U) ||
        ((now - g_thermal_last_frame_tick) > 1500U))
    {
        actual_fps_x10 = 0U;
    }

    lv_label_set_text_fmt(guider_ui.screen_label_1,
                          "\xE4\xB8\xAD\xE5\xBF\x83:%d.%02d\xE2\x84\x83    \xE6\x9C\x80\xE9\xAB\x98:%d.%02d\xE2\x84\x83 ",
                          center_x100 / 100, abs(center_x100 % 100),
                          maximum_x100 / 100, abs(maximum_x100 % 100));
    lv_label_set_text_fmt(guider_ui.screen_label_3,
                          "设置:%uHz  热图:%u.%uFPS",
                          refresh_rate_to_hz(refresh_rate),
                          actual_fps_x10 / 10U, actual_fps_x10 % 10U);
}

void events_init_screen (lv_ui *ui)
{
    battery_screen_create();
    lv_obj_add_event_cb(ui->screen_btn_5, screen_btn_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_btn_6, screen_btn_6_event_handler, LV_EVENT_ALL, ui);
    lv_label_set_text(ui->screen_btn_7_label, "\xE7\x94\xB5\xE9\x87\x8F");
    lv_obj_add_event_cb(ui->screen_btn_7, screen_btn_7_event_handler, LV_EVENT_ALL, ui);
     // 添加定时器定期更新温度显示
    lv_timer_create(update_temperature_handler, 500, NULL); // 每500毫秒刷新温度和帧率文本
}


void events_init(lv_ui *ui)
{

}
