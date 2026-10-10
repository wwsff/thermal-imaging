#include "boot_screen.h"

#include "boot_logo.h"
#include "gui_guider.h"

static lv_obj_t *boot_screen;
static lv_obj_t *boot_status_label;
static lv_obj_t *boot_detail_label;
static lv_obj_t *boot_progress_bar;
static lv_obj_t *boot_percent_label;

void boot_screen_create(void)
{
    lv_obj_t *logo;
    lv_obj_t *title;

    boot_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(boot_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(boot_screen, lv_color_hex(0x101214), 0);
    lv_obj_set_style_bg_opa(boot_screen, LV_OPA_COVER, 0);

    logo = lv_img_create(boot_screen);
    lv_img_set_src(logo, &boot_logo);
    lv_obj_set_style_img_recolor(logo, lv_color_hex(0xF2F2F2), 0);
    lv_obj_set_style_img_recolor_opa(logo, LV_OPA_COVER, 0);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 3);

    title = lv_label_create(boot_screen);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_SourceHanSansCN_Light_2_16, 0);
    lv_label_set_text(title, "\xE7\xB3\xBB\xE7\xBB\x9F\xE8\x87\xAA\xE6\xA3\x80");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 109);

    boot_status_label = lv_label_create(boot_screen);
    lv_obj_set_width(boot_status_label, 290);
    lv_obj_set_style_text_align(boot_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(boot_status_label, lv_color_hex(0xE8EDF2), 0);
    lv_obj_set_style_text_font(boot_status_label, &lv_font_SourceHanSansCN_Light_2_12, 0);
    lv_label_set_long_mode(boot_status_label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(boot_status_label, "");
    lv_obj_align(boot_status_label, LV_ALIGN_TOP_MID, 0, 135);

    boot_detail_label = lv_label_create(boot_screen);
    lv_obj_set_width(boot_detail_label, 290);
    lv_obj_set_style_text_align(boot_detail_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(boot_detail_label, lv_color_hex(0x8F9BA7), 0);
    lv_obj_set_style_text_font(boot_detail_label, &lv_font_SourceHanSansCN_Light_2_12, 0);
    lv_label_set_long_mode(boot_detail_label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(boot_detail_label, "");
    lv_obj_align(boot_detail_label, LV_ALIGN_TOP_MID, 0, 153);

    boot_progress_bar = lv_bar_create(boot_screen);
    lv_obj_set_size(boot_progress_bar, 258, 12);
    lv_bar_set_range(boot_progress_bar, 0, 100);
    lv_bar_set_value(boot_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(boot_progress_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_progress_bar, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(boot_progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_progress_bar, 6, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(boot_progress_bar, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(boot_progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align(boot_progress_bar, LV_ALIGN_TOP_MID, 0, 178);

    boot_percent_label = lv_label_create(boot_screen);
    lv_obj_set_width(boot_percent_label, 80);
    lv_obj_set_style_text_align(boot_percent_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(boot_percent_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(boot_percent_label, &lv_font_montserratMedium_12, 0);
    lv_label_set_text(boot_percent_label, "0%");
    lv_obj_align(boot_percent_label, LV_ALIGN_TOP_MID, 0, 197);

    lv_scr_load(boot_screen);
}

void boot_screen_set_progress(uint8_t percent, const char *status,
                              const char *detail)
{
    if (boot_screen == NULL)
    {
        return;
    }
    if (percent > 100U)
    {
        percent = 100U;
    }

    lv_bar_set_value(boot_progress_bar, percent, LV_ANIM_ON);
    lv_label_set_text(boot_status_label, (status != NULL) ? status : "");
    lv_label_set_text(boot_detail_label, (detail != NULL) ? detail : "");
    lv_label_set_text_fmt(boot_percent_label, "%u%%", (unsigned int)percent);
}

void boot_screen_close(lv_obj_t *next_screen)
{
    lv_obj_t *old_screen = boot_screen;

    if ((old_screen == NULL) || (next_screen == NULL))
    {
        return;
    }

    lv_scr_load(next_screen);
    boot_screen = NULL;
    boot_status_label = NULL;
    boot_detail_label = NULL;
    boot_progress_bar = NULL;
    boot_percent_label = NULL;
    lv_obj_del(old_screen);
}
