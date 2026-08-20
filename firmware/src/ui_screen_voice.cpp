// Project  : House Phone
// File     : ui_screen_voice.cpp
// Purpose  : "Jarvis Voice Capture" tile — record button + last AI confirmation
// Depends  : ui_screen_voice.h

#include "ui_screen_voice.h"
#include "ui.h"
#include "plaud_mode.h"

static lv_obj_t * s_feed_label = nullptr;
static lv_obj_t * s_record_btn = nullptr;
static lv_obj_t * s_record_lbl = nullptr;

static void set_record_button_state(bool recording)
{
    if (!s_record_btn) return;
    lv_obj_set_style_bg_color(s_record_btn, lv_color_hex(recording ? 0xF44336 : UI_CLR_ACCENT), 0);
    lv_label_set_text(s_record_lbl, recording ? LV_SYMBOL_STOP "  Stop" : LV_SYMBOL_AUDIO "  Record");
}

static void record_btn_event_cb(lv_event_t * e)
{
    (void)e;
    set_record_button_state(plaudModeToggleManualRecording());
}

void uiVoiceScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "JARVIS VOICE CAPTURE");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 14);

    s_feed_label = lv_label_create(tile);
    lv_label_set_long_mode(s_feed_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_feed_label, UI_SCREEN_W - 60);
    lv_label_set_text(s_feed_label, "Jarvis is ready.\nHold BOOT or tap Record to capture a thought.");
    lv_obj_set_style_text_align(s_feed_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_feed_label, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_text_font(s_feed_label, &lv_font_montserrat_22, 0);
    lv_obj_align(s_feed_label, LV_ALIGN_TOP_MID, 0, 50);

    // Large, easy-to-hit record toggle — the status bar's mic icon was too
    // small to reliably tap. Doesn't touch the backlight (that's BOOT's job).
    s_record_btn = lv_obj_create(tile);
    lv_obj_set_size(s_record_btn, 220, 64);
    lv_obj_align(s_record_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_remove_flag(s_record_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_record_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(s_record_btn, 16, 0);
    lv_obj_set_style_border_width(s_record_btn, 0, 0);
    lv_obj_set_style_bg_color(s_record_btn, lv_color_hex(UI_CLR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_record_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_record_btn, lv_color_mix(lv_color_black(), lv_color_hex(UI_CLR_ACCENT), LV_OPA_20),
                              LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(s_record_btn, record_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    s_record_lbl = lv_label_create(s_record_btn);
    lv_label_set_text(s_record_lbl, LV_SYMBOL_AUDIO "  Record");
    lv_obj_set_style_text_color(s_record_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_record_lbl, &lv_font_montserrat_22, 0);
    lv_obj_center(s_record_lbl);
}

void uiVoiceSetText(const char * text)
{
    if(!s_feed_label || !text) return;
    lv_label_set_text(s_feed_label, text);
}
