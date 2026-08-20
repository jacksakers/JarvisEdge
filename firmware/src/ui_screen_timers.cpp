// Project  : House Phone
// File     : ui_screen_timers.cpp
// Purpose  : "Timers & Alarms" tile — cooking presets + bedside alarm config
// Depends  : ui_screen_timers.h, ui.h, timers_alarms.h

#include "ui_screen_timers.h"
#include "ui.h"
#include "timers_alarms.h"
#include <Arduino.h>
#include <stdio.h>

static lv_obj_t * s_countdown_lbl = nullptr;
static lv_obj_t * s_ring_tile     = nullptr;
static lv_obj_t * s_hour_roller   = nullptr;
static lv_obj_t * s_minute_roller = nullptr;
static lv_obj_t * s_alarm_switch  = nullptr;
static lv_obj_t * s_alarm_status_lbl = nullptr;

static void preset_ev(lv_event_t * e)
{
    int seconds = (int)(intptr_t)lv_event_get_user_data(e);
    timersAlarmsStartCountdown(seconds);
}

static void cancel_ev(lv_event_t * e)
{
    (void)e;
    timersAlarmsCancelCountdown();
}

static void ring_tile_clicked_ev(lv_event_t * e)
{
    (void)e;
    if (timersAlarmsIsRinging()) timersAlarmsSilence();
}

static void alarm_save_ev(lv_event_t * e)
{
    (void)e;
    int hour = lv_roller_get_selected(s_hour_roller);
    int minute = lv_roller_get_selected(s_minute_roller);
    bool enabled = lv_obj_has_state(s_alarm_switch, LV_STATE_CHECKED);
    timersAlarmsSetAlarm(hour, minute, enabled);
    lv_label_set_text_fmt(s_alarm_status_lbl, "Alarm %02d:%02d %s", hour, minute,
                           enabled ? "enabled" : "saved (disabled)");
}

// "00\n01\n02\n...\n23" and "00\n05\n10\n...\n55" option strings, built once.
static void build_hour_options(char * buf, size_t buf_len)
{
    buf[0] = '\0';
    for (int h = 0; h < 24; h++) {
        char item[4];
        snprintf(item, sizeof(item), "%02d\n", h);
        strncat(buf, item, buf_len - strlen(buf) - 1);
    }
}

static void build_minute_options(char * buf, size_t buf_len)
{
    buf[0] = '\0';
    for (int m = 0; m < 60; m += 5) {
        char item[4];
        snprintf(item, sizeof(item), "%02d\n", m);
        strncat(buf, item, buf_len - strlen(buf) - 1);
    }
}

void uiTimersScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "TIMERS & ALARMS");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 6);

    // Full-tile invisible overlay used only to catch a tap-to-silence while
    // ringing — sits above everything, hidden the rest of the time.
    s_ring_tile = lv_obj_create(tile);
    lv_obj_set_size(s_ring_tile, UI_SCREEN_W, UI_CAROUSEL_H);
    lv_obj_set_pos(s_ring_tile, 0, 0);
    lv_obj_remove_flag(s_ring_tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_ring_tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(s_ring_tile, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_bg_opa(s_ring_tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ring_tile, 0, 0);
    lv_obj_add_flag(s_ring_tile, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_ring_tile, ring_tile_clicked_ev, LV_EVENT_CLICKED, nullptr);

    lv_obj_t * ring_lbl = lv_label_create(s_ring_tile);
    lv_label_set_text(ring_lbl, LV_SYMBOL_BELL "  Tap to silence");
    lv_obj_set_style_text_font(ring_lbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(ring_lbl, lv_color_white(), 0);
    lv_obj_center(ring_lbl);

    // ── Countdown ──────────────────────────────────────────────────────────
    s_countdown_lbl = lv_label_create(tile);
    lv_label_set_text(s_countdown_lbl, "--:--");
    lv_obj_set_style_text_font(s_countdown_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_countdown_lbl, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_align(s_countdown_lbl, LV_ALIGN_TOP_MID, 0, 28);

    static const struct { const char * label; int seconds; } presets[] = {
        { "+5 Min",  5 * 60 },
        { "+15 Min", 15 * 60 },
        { "Pasta 10m", 10 * 60 },
    };
    const int preset_w = (UI_SCREEN_W - 40 - 2 * 10) / 3;
    for (int i = 0; i < 3; i++) {
        lv_obj_t * btn = lv_button_create(tile);
        lv_obj_set_size(btn, preset_w, 40);
        lv_obj_set_pos(btn, 20 + i * (preset_w + 10), 78);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_CLR_ACCENT), 0);
        lv_obj_add_event_cb(btn, preset_ev, LV_EVENT_CLICKED, (void *)(intptr_t)presets[i].seconds);
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, presets[i].label);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
    }

    lv_obj_t * cancel_btn = lv_button_create(tile);
    lv_obj_set_size(cancel_btn, 100, 26);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_MID, 0, 126);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x2A2A3A), 0);
    lv_obj_add_event_cb(cancel_btn, cancel_ev, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, LV_SYMBOL_CLOSE "  Cancel");
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(cancel_lbl);

    // ── Bedside alarm ────────────────────────────────────────────────────
    lv_obj_t * alarm_hint = lv_label_create(tile);
    lv_label_set_text(alarm_hint, "Bedside Alarm");
    lv_obj_set_style_text_color(alarm_hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(alarm_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(alarm_hint, LV_ALIGN_TOP_LEFT, 20, 168);

    static char hour_opts[24 * 4];
    static char minute_opts[12 * 4];
    build_hour_options(hour_opts, sizeof(hour_opts));
    build_minute_options(minute_opts, sizeof(minute_opts));

    s_hour_roller = lv_roller_create(tile);
    lv_roller_set_options(s_hour_roller, hour_opts, LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(s_hour_roller, 70, 60);
    lv_obj_align(s_hour_roller, LV_ALIGN_TOP_LEFT, 20, 190);
    lv_obj_set_style_text_font(s_hour_roller, &lv_font_montserrat_12, 0);
    lv_obj_set_style_bg_color(s_hour_roller, lv_color_hex(UI_CLR_SURFACE), 0);
    lv_obj_set_style_text_color(s_hour_roller, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_bg_color(s_hour_roller, lv_color_hex(UI_CLR_ACCENT), LV_PART_SELECTED);

    s_minute_roller = lv_roller_create(tile);
    lv_roller_set_options(s_minute_roller, minute_opts, LV_ROLLER_MODE_INFINITE);
    lv_obj_set_size(s_minute_roller, 70, 60);
    lv_obj_align(s_minute_roller, LV_ALIGN_TOP_LEFT, 100, 190);
    lv_obj_set_style_text_font(s_minute_roller, &lv_font_montserrat_12, 0);
    lv_obj_set_style_bg_color(s_minute_roller, lv_color_hex(UI_CLR_SURFACE), 0);
    lv_obj_set_style_text_color(s_minute_roller, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_bg_color(s_minute_roller, lv_color_hex(UI_CLR_ACCENT), LV_PART_SELECTED);

    s_alarm_switch = lv_switch_create(tile);
    lv_obj_align(s_alarm_switch, LV_ALIGN_TOP_LEFT, 190, 205);
    lv_obj_set_style_bg_color(s_alarm_switch, lv_color_hex(UI_CLR_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t * save_btn = lv_button_create(tile);
    lv_obj_set_size(save_btn, 90, 30);
    lv_obj_align(save_btn, LV_ALIGN_TOP_RIGHT, -20, 200);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x1A5C1A), 0);
    lv_obj_add_event_cb(save_btn, alarm_save_ev, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, LV_SYMBOL_SAVE " Set");
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(save_lbl);

    s_alarm_status_lbl = lv_label_create(tile);
    lv_label_set_text(s_alarm_status_lbl, "No alarm set.");
    lv_obj_set_style_text_color(s_alarm_status_lbl, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(s_alarm_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align(s_alarm_status_lbl, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Seed the roller/switch/status from whatever settings.cpp restored.
    int hour, minute;
    bool enabled;
    timersAlarmsGetAlarm(&hour, &minute, &enabled);
    if (hour >= 0) {
        lv_roller_set_selected(s_hour_roller, hour, LV_ANIM_OFF);
        lv_roller_set_selected(s_minute_roller, minute / 5, LV_ANIM_OFF);
        if (enabled) lv_obj_add_state(s_alarm_switch, LV_STATE_CHECKED);
        lv_label_set_text_fmt(s_alarm_status_lbl, "Alarm %02d:%02d %s", hour, minute,
                               enabled ? "enabled" : "disabled");
    }
}

void uiTimersScreenHandle(unsigned long now)
{
    (void)now;
    int remaining = timersAlarmsGetRemainingSeconds();
    if (remaining >= 0) {
        lv_label_set_text_fmt(s_countdown_lbl, "%02d:%02d", remaining / 60, remaining % 60);
    } else if (!timersAlarmsIsRinging()) {
        lv_label_set_text(s_countdown_lbl, "--:--");
    }

    bool ringing = timersAlarmsIsRinging();
    bool overlay_visible = !lv_obj_has_flag(s_ring_tile, LV_OBJ_FLAG_HIDDEN);
    if (ringing && !overlay_visible) {
        lv_obj_remove_flag(s_ring_tile, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_ring_tile);
    } else if (!ringing && overlay_visible) {
        lv_obj_add_flag(s_ring_tile, LV_OBJ_FLAG_HIDDEN);
    }
}
