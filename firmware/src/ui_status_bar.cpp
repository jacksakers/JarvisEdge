// Project  : House Phone
// File     : ui_status_bar.cpp
// Purpose  : Persistent top status bar — battery, WiFi, LoRa, queue indicators
// Depends  : ui_status_bar.h

#include "ui_status_bar.h"
#include "ui.h"

static lv_obj_t * s_bar        = nullptr;
static lv_obj_t * s_wifi_icon  = nullptr;
static lv_obj_t * s_lora_icon  = nullptr;
static lv_obj_t * s_batt_label = nullptr;
static lv_obj_t * s_queue_badge = nullptr;

void uiStatusBarInit(lv_obj_t * parent)
{
    s_bar = lv_obj_create(parent);
    lv_obj_set_size(s_bar, UI_SCREEN_W, UI_STATUSBAR_H);
    lv_obj_set_pos(s_bar, 0, 0);
    lv_obj_remove_flag(s_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(UI_CLR_SURFACE), 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_bar, 0, 0);
    lv_obj_set_style_radius(s_bar, 0, 0);
    lv_obj_set_style_pad_hor(s_bar, 10, 0);
    lv_obj_set_style_pad_ver(s_bar, 0, 0);

    lv_obj_t * title = lv_label_create(s_bar);
    lv_label_set_text(title, "JARVIS");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    // Right-to-left cluster: battery, queue badge, LoRa, WiFi
    s_wifi_icon = lv_label_create(s_bar);
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_wifi_icon, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_align(s_wifi_icon, LV_ALIGN_RIGHT_MID, 0, 0);

    s_lora_icon = lv_label_create(s_bar);
    lv_label_set_text(s_lora_icon, LV_SYMBOL_SHUFFLE);   // stand-in glyph for LoRa link
    lv_obj_set_style_text_color(s_lora_icon, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_align(s_lora_icon, LV_ALIGN_RIGHT_MID, -26, 0);

    s_queue_badge = lv_label_create(s_bar);
    lv_label_set_text(s_queue_badge, "");
    lv_obj_set_style_text_color(s_queue_badge, lv_color_hex(0xFF9800), 0);
    lv_obj_align(s_queue_badge, LV_ALIGN_RIGHT_MID, -52, 0);

    s_batt_label = lv_label_create(s_bar);
    lv_label_set_text(s_batt_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(s_batt_label, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_align(s_batt_label, LV_ALIGN_RIGHT_MID, -90, 0);
}

void uiStatusBarSetWifiConnected(bool connected)
{
    if(!s_wifi_icon) return;
    lv_obj_set_style_text_color(s_wifi_icon,
        lv_color_hex(connected ? 0x4CAF50 : UI_CLR_MUTED), 0);
}

void uiStatusBarSetLoraConnected(bool connected)
{
    if(!s_lora_icon) return;
    lv_obj_set_style_text_color(s_lora_icon,
        lv_color_hex(connected ? 0x4CAF50 : UI_CLR_MUTED), 0);
}

void uiStatusBarSetBatteryPercent(int percent)
{
    if(!s_batt_label) return;
    if(percent < 0) {
        lv_label_set_text(s_batt_label, LV_SYMBOL_BATTERY_FULL);
        lv_obj_set_style_text_color(s_batt_label, lv_color_hex(UI_CLR_MUTED), 0);
        return;
    }
    const char * symbol = LV_SYMBOL_BATTERY_EMPTY;
    if(percent > 87)      symbol = LV_SYMBOL_BATTERY_FULL;
    else if(percent > 62) symbol = LV_SYMBOL_BATTERY_3;
    else if(percent > 37) symbol = LV_SYMBOL_BATTERY_2;
    else if(percent > 12) symbol = LV_SYMBOL_BATTERY_1;
    lv_label_set_text(s_batt_label, symbol);
    lv_obj_set_style_text_color(s_batt_label,
        lv_color_hex(percent > 20 ? UI_CLR_TEXT : 0xF44336), 0);
}

void uiStatusBarSetQueueCount(int count)
{
    if(!s_queue_badge) return;
    if(count <= 0) {
        lv_label_set_text(s_queue_badge, "");
    } else {
        lv_label_set_text_fmt(s_queue_badge, LV_SYMBOL_UPLOAD " %d", count);
    }
}
