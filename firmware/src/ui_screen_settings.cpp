// Project  : Jarvis Edge Node
// File     : ui_screen_settings.cpp
// Purpose  : "Settings" tile — on-device WiFi/backend/MQTT configuration
// Depends  : ui_screen_settings.h, ui.h, settings.h, wifi_manager.h, mqtt_client.h
//
// A scrollable form of textareas + an on-screen keyboard overlay, mirroring
// the pattern used by HelpDesk's WiFi settings screen (ui_Screen12.cpp).
// Saving writes to /settings/jarvis.txt on SD (settings.cpp) and immediately
// reconnects WiFi/MQTT so changes take effect without a reboot.

#include "ui_screen_settings.h"
#include "ui.h"
#include "settings.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <stdio.h>

#define ROW_H       50
#define TA_H        30
#define FORM_Y      24
#define SAVE_BTN_H  32
#define KBD_H      130

static lv_obj_t * s_ssid_ta   = nullptr;
static lv_obj_t * s_pass_ta   = nullptr;
static lv_obj_t * s_bhost_ta  = nullptr;
static lv_obj_t * s_bport_ta  = nullptr;
static lv_obj_t * s_mhost_ta  = nullptr;
static lv_obj_t * s_mport_ta  = nullptr;
static lv_obj_t * s_kbd       = nullptr;
static lv_obj_t * s_status_lbl = nullptr;

static void ta_focused_ev(lv_event_t * e)
{
    lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);
    bool numeric = (ta == s_bport_ta || ta == s_mport_ta);
    lv_keyboard_set_mode(s_kbd, numeric ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_kbd, ta);
    lv_obj_remove_flag(s_kbd, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_kbd);
    lv_obj_scroll_to_view(ta, LV_ANIM_ON);
}

static void kbd_ready_ev(lv_event_t * e)
{
    (void)e;
    lv_obj_add_flag(s_kbd, LV_OBJ_FLAG_HIDDEN);
}

static void save_btn_ev(lv_event_t * e)
{
    (void)e;
    lv_obj_add_flag(s_kbd, LV_OBJ_FLAG_HIDDEN);

    settingsSetWifiSSID(lv_textarea_get_text(s_ssid_ta));
    settingsSetWifiPassword(lv_textarea_get_text(s_pass_ta));
    settingsSetBackendHost(lv_textarea_get_text(s_bhost_ta));
    settingsSetBackendPort(atoi(lv_textarea_get_text(s_bport_ta)));
    settingsSetMqttHost(lv_textarea_get_text(s_mhost_ta));
    settingsSetMqttPort(atoi(lv_textarea_get_text(s_mport_ta)));
    settingsSave();

    wifiManagerReconnect();
    mqttClientReconnect();

    if (s_status_lbl) lv_label_set_text(s_status_lbl, "Saved — reconnecting...");
}

static lv_obj_t * build_field(lv_obj_t * parent, const char * label_text, int y,
                               const char * initial, bool is_password)
{
    lv_obj_t * lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_pos(lbl, 4, y);
    lv_obj_set_style_text_color(lbl, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);

    lv_obj_t * ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, UI_SCREEN_W - 32, TA_H);
    lv_obj_set_pos(ta, 4, y + 16);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, initial ? initial : "");
    lv_textarea_set_password_mode(ta, is_password);
    lv_obj_set_style_bg_color(ta, lv_color_hex(UI_CLR_SURFACE), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(UI_CLR_ACCENT), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 6, 0);
    lv_obj_set_style_pad_hor(ta, 8, 0);
    lv_obj_add_event_cb(ta, ta_focused_ev, LV_EVENT_FOCUSED, NULL);
    return ta;
}

void uiSettingsScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "SETTINGS");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 4);

    int form_h = UI_CAROUSEL_H - FORM_Y - SAVE_BTN_H - 8;
    lv_obj_t * form = lv_obj_create(tile);
    lv_obj_set_size(form, UI_SCREEN_W, form_h);
    lv_obj_set_pos(form, 0, FORM_Y);
    lv_obj_set_style_bg_opa(form, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(form, 0, 0);
    lv_obj_set_style_pad_all(form, 0, 0);
    lv_obj_set_scroll_dir(form, LV_DIR_VER);

    char port_buf[8];

    s_ssid_ta  = build_field(form, "WiFi SSID", 0 * ROW_H, settingsGetWifiSSID(), false);
    s_pass_ta  = build_field(form, "WiFi Password", 1 * ROW_H, settingsGetWifiPassword(), true);
    s_bhost_ta = build_field(form, "Backend Host (home server IP)", 2 * ROW_H, settingsGetBackendHost(), false);
    snprintf(port_buf, sizeof(port_buf), "%d", settingsGetBackendPort());
    s_bport_ta = build_field(form, "Backend Port", 3 * ROW_H, port_buf, false);
    s_mhost_ta = build_field(form, "MQTT Broker Host", 4 * ROW_H, settingsGetMqttHost(), false);
    snprintf(port_buf, sizeof(port_buf), "%d", settingsGetMqttPort());
    s_mport_ta = build_field(form, "MQTT Broker Port", 5 * ROW_H, port_buf, false);

    lv_obj_t * save_btn = lv_button_create(tile);
    lv_obj_set_size(save_btn, 160, SAVE_BTN_H);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x1A5C1A), 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x2E7D32), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(save_btn, 8, 0);
    lv_obj_add_event_cb(save_btn, save_btn_ev, LV_EVENT_CLICKED, NULL);

    lv_obj_t * save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, LV_SYMBOL_SAVE "  Save & Reconnect");
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(save_lbl);

    s_status_lbl = lv_label_create(tile);
    lv_label_set_text(s_status_lbl, "");
    lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(s_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align_to(s_status_lbl, save_btn, LV_ALIGN_OUT_TOP_MID, 0, -4);

    s_kbd = lv_keyboard_create(tile);
    lv_obj_set_size(s_kbd, UI_SCREEN_W, KBD_H);
    lv_obj_align(s_kbd, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kbd, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_kbd, kbd_ready_ev, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_kbd, kbd_ready_ev, LV_EVENT_CANCEL, NULL);
}
