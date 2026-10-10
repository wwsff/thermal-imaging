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
#include "lvgl.h"

#include "color.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

extern color_mode dis_mode;
extern float max_temp, min_temp, cent_temp;

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
            lv_label_set_text(guider_ui.screen_label_2, "当前显示模式:伪彩模式");
        }
        else if(dis_mode == white_hot_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "白热模式");
            lv_label_set_text(guider_ui.screen_label_2, "当前显示模式:白热模式");
        }
        else if(dis_mode == black_hot_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "黑热模式");
            lv_label_set_text(guider_ui.screen_label_2, "当前显示模式:黑热模式");
        }
        else if(dis_mode == medical_mode)
        {
            //lv_label_set_text(guider_ui.screen_btn_5_label, "医疗模式");
            lv_label_set_text(guider_ui.screen_label_2, "当前显示模式:医疗模式");
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
        break;
    }
    default:
        break;
    }
}
static void update_temperature_handler(lv_event_t *e)
{
    
    // 创建格式化字符串
    lv_label_set_text_fmt(guider_ui.screen_label_1, "中心温度(℃): %d.%d   最高温度(℃): %d.%d", \
        (int32_t)cent_temp,(int32_t)((cent_temp-(int32_t)cent_temp)*100), (int32_t)max_temp,(int32_t)((max_temp-(int32_t)max_temp)*100));
}

void events_init_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_btn_5, screen_btn_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_btn_6, screen_btn_6_event_handler, LV_EVENT_ALL, ui);
    // 添加定时器定期更新温度显示
    lv_timer_create(update_temperature_handler, 500, NULL); // 每秒更新一次
}


void events_init(lv_ui *ui)
{

}
