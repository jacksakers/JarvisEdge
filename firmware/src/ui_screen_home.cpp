// Project  : House Phone
// File     : ui_screen_home.cpp
// Purpose  : "Ambient Home" tile — grid of Tapo bulb zones
// Depends  : ui_screen_home.h, ui.h, tapo_control.h
//
// Tap a zone tile to toggle it (optimistic — the next tapo_control poll
// reconciles reality). Long-press opens a full-tile brightness slider
// overlay, mirroring the keyboard-overlay pattern used elsewhere
// (ui_screen_settings.cpp / the old ui_screen_actions.cpp).

#include "ui_screen_home.h"
#include "ui.h"
#include <Arduino.h>
#include <string.h>

#define GRID_COLS   3
#define GRID_ROWS   2

static lv_obj_t * s_tiles[TAPO_MAX_ZONES];
static lv_obj_t * s_name_labels[TAPO_MAX_ZONES];
static lv_obj_t * s_state_labels[TAPO_MAX_ZONES];
static int        s_zone_ids[TAPO_MAX_ZONES];
static int        s_zone_brightness[TAPO_MAX_ZONES];
static bool        s_long_press_fired = false;

static lv_obj_t * s_empty_hint = nullptr;

// ── Brightness overlay ────────────────────────────────────────────────────
static lv_obj_t * s_overlay     = nullptr;
static lv_obj_t * s_overlay_title = nullptr;
static lv_obj_t * s_overlay_slider = nullptr;
static int        s_overlay_idx  = -1;

static void hide_overlay(lv_event_t * e)
{
    (void)e;
    if (s_overlay) lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void slider_released_ev(lv_event_t * e)
{
    (void)e;
    if (s_overlay_idx < 0) return;
    int brightness = (int)lv_slider_get_value(s_overlay_slider);
    s_zone_brightness[s_overlay_idx] = brightness;
    tapoControlSetBrightness(s_zone_ids[s_overlay_idx], brightness);
}

static void build_overlay(lv_obj_t * tile)
{
    s_overlay = lv_obj_create(tile);
    lv_obj_set_size(s_overlay, UI_SCREEN_W, UI_CAROUSEL_H);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    s_overlay_title = lv_label_create(s_overlay);
    lv_obj_set_style_text_color(s_overlay_title, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_text_font(s_overlay_title, &lv_font_montserrat_16, 0);
    lv_obj_align(s_overlay_title, LV_ALIGN_TOP_MID, 0, 20);

    s_overlay_slider = lv_slider_create(s_overlay);
    lv_obj_set_size(s_overlay_slider, UI_SCREEN_W - 80, 16);
    lv_obj_align(s_overlay_slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s_overlay_slider, 1, 100);
    lv_obj_set_style_bg_color(s_overlay_slider, lv_color_hex(UI_CLR_ACCENT), LV_PART_KNOB);
    lv_obj_set_style_bg_color(s_overlay_slider, lv_color_hex(UI_CLR_ACCENT), LV_PART_INDICATOR);
    lv_obj_add_event_cb(s_overlay_slider, slider_released_ev, LV_EVENT_RELEASED, nullptr);

    lv_obj_t * close_btn = lv_button_create(s_overlay);
    lv_obj_set_size(close_btn, UI_SCREEN_W - 60, 32);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x2A2A3A), 0);
    lv_obj_add_event_cb(close_btn, hide_overlay, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_OK "  Done");
    lv_obj_center(close_lbl);
}

static void show_overlay(int idx)
{
    s_overlay_idx = idx;
    lv_label_set_text(s_overlay_title, lv_label_get_text(s_name_labels[idx]));
    lv_slider_set_value(s_overlay_slider, s_zone_brightness[idx], LV_ANIM_OFF);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
}

// ── Grid tiles ────────────────────────────────────────────────────────────

static void set_tile_style(int idx, bool on, bool reachable)
{
    lv_obj_t * t = s_tiles[idx];
    if (!reachable) {
        lv_obj_set_style_bg_color(t, lv_color_hex(0x3A2A2A), 0);
    } else if (on) {
        lv_obj_set_style_bg_color(t, lv_color_hex(0xFFC107), 0);
    } else {
        lv_obj_set_style_bg_color(t, lv_color_hex(UI_CLR_SURFACE), 0);
    }
    lv_obj_set_style_text_color(s_name_labels[idx],
        lv_color_hex((on && reachable) ? 0x1A1A1A : UI_CLR_TEXT), 0);
    lv_obj_set_style_text_color(s_state_labels[idx],
        lv_color_hex((on && reachable) ? 0x1A1A1A : UI_CLR_MUTED), 0);
    lv_label_set_text(s_state_labels[idx], !reachable ? "unreachable" : (on ? "on" : "off"));
}

static void tile_long_pressed_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_zone_ids[idx] < 0) return;
    s_long_press_fired = true;
    show_overlay(idx);
}

static void tile_clicked_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_long_press_fired) {
        s_long_press_fired = false;   // this click was the tail end of a long-press
        return;
    }
    if (s_zone_ids[idx] < 0) return;
    tapoControlToggle(s_zone_ids[idx]);
}

static void all_off_ev(lv_event_t * e)
{
    (void)e;
    tapoControlAllOff();
}

void uiHomeScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "AMBIENT HOME");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 12, 10);

    lv_obj_t * all_off_btn = lv_button_create(tile);
    lv_obj_set_size(all_off_btn, 76, 22);
    lv_obj_align(all_off_btn, LV_ALIGN_TOP_RIGHT, -8, 6);
    lv_obj_set_style_bg_color(all_off_btn, lv_color_hex(0x442222), 0);
    lv_obj_add_event_cb(all_off_btn, all_off_ev, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * all_off_lbl = lv_label_create(all_off_btn);
    lv_label_set_text(all_off_lbl, "All Off");
    lv_obj_set_style_text_font(all_off_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(all_off_lbl);

    s_empty_hint = lv_label_create(tile);
    lv_label_set_text(s_empty_hint, "No zones yet.\nAdd one from the Command Center's\nAmbient Home page.");
    lv_obj_set_style_text_align(s_empty_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_empty_hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(s_empty_hint, &lv_font_montserrat_14, 0);
    lv_obj_center(s_empty_hint);

    const int gap = 10, margin = 12, top = 34;
    const int tile_w = (UI_SCREEN_W - margin * 2 - gap * (GRID_COLS - 1)) / GRID_COLS;
    const int tile_h = (UI_CAROUSEL_H - top - 8 - gap * (GRID_ROWS - 1)) / GRID_ROWS;

    for (int i = 0; i < TAPO_MAX_ZONES; i++) {
        int col = i % GRID_COLS, row = i / GRID_COLS;
        int x = margin + col * (tile_w + gap);
        int y = top + row * (tile_h + gap);

        lv_obj_t * t = lv_obj_create(tile);
        lv_obj_set_size(t, tile_w, tile_h);
        lv_obj_set_pos(t, x, y);
        lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(t, lv_color_hex(UI_CLR_SURFACE), 0);
        lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(t, 0, 0);
        lv_obj_set_style_radius(t, 10, 0);
        lv_obj_add_flag(t, LV_OBJ_FLAG_HIDDEN);   // hidden until a real zone fills this slot
        lv_obj_add_event_cb(t, tile_clicked_ev, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(t, tile_long_pressed_ev, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);
        s_tiles[i] = t;

        lv_obj_t * name_lbl = lv_label_create(t);
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name_lbl, tile_w - 12);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(UI_CLR_TEXT), 0);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_MID, 0, 8);
        s_name_labels[i] = name_lbl;

        lv_obj_t * state_lbl = lv_label_create(t);
        lv_obj_set_style_text_font(state_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(state_lbl, lv_color_hex(UI_CLR_MUTED), 0);
        lv_obj_align(state_lbl, LV_ALIGN_BOTTOM_MID, 0, -8);
        s_state_labels[i] = state_lbl;

        s_zone_ids[i] = -1;
        s_zone_brightness[i] = 100;
    }

    build_overlay(tile);
}

void uiHomeSetZones(const TapoZoneState * zones, int count)
{
    if (count > TAPO_MAX_ZONES) count = TAPO_MAX_ZONES;

    if (s_empty_hint) {
        if (count == 0) lv_obj_remove_flag(s_empty_hint, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_empty_hint, LV_OBJ_FLAG_HIDDEN);
    }

    for (int i = 0; i < TAPO_MAX_ZONES; i++) {
        if (i < count) {
            s_zone_ids[i] = zones[i].id;
            s_zone_brightness[i] = zones[i].brightness;
            lv_label_set_text(s_name_labels[i], zones[i].name);
            set_tile_style(i, zones[i].on, zones[i].reachable);
            lv_obj_remove_flag(s_tiles[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            s_zone_ids[i] = -1;
            lv_obj_add_flag(s_tiles[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
