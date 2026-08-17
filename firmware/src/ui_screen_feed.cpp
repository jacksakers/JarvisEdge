// Project  : Jarvis Edge Node
// File     : ui_screen_feed.cpp
// Purpose  : "Jarvis Feed" home tile — most recent AI confirmation/alert
// Depends  : ui_screen_feed.h

#include "ui_screen_feed.h"
#include "ui.h"

static lv_obj_t * s_feed_label = nullptr;

void uiFeedScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "JARVIS FEED");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 14);

    s_feed_label = lv_label_create(tile);
    lv_label_set_long_mode(s_feed_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_feed_label, UI_SCREEN_W - 60);
    lv_label_set_text(s_feed_label, "Jarvis is ready.\nHold BOOT to capture a thought.");
    lv_obj_set_style_text_align(s_feed_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_feed_label, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_text_font(s_feed_label, &lv_font_montserrat_22, 0);
    lv_obj_center(s_feed_label);
}

void uiFeedSetText(const char * text)
{
    if(!s_feed_label || !text) return;
    lv_label_set_text(s_feed_label, text);
}
