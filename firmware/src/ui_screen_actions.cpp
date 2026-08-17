// Project  : Jarvis Edge Node
// File     : ui_screen_actions.cpp
// Purpose  : "Action Grid" tile — 2x2 grid of manual trigger buttons
// Depends  : ui_screen_actions.h, edge_api.h
//
// Time Track / Dismiss fire immediately (POST /actions/{type}, no body).
// Note / Alert pop a full-tile keyboard overlay to collect text first, then
// POST the same way. The backend publishes the resulting Jarvis Feed text
// back over MQTT (see app/main.py trigger_action()), so this file doesn't
// need to touch uiFeedSetText directly — it only gives immediate tap
// feedback via a short opacity "flash" animation.

#include "ui_screen_actions.h"
#include "ui.h"
#include "edge_api.h"
#include <Arduino.h>
#include <string.h>

typedef struct {
    const char * label;
    const char * action_type;   // matches VALID_ACTION_TYPES in app/main.py
    uint32_t     color_hex;
    bool         needs_text;
} action_tile_t;

static const action_tile_t k_actions[] = {
    { LV_SYMBOL_CALL    "\nTime Track", "time_track", 0x2196F3, false },
    { LV_SYMBOL_EDIT    "\nNote",       "note",       0x4CAF50, true  },
    { LV_SYMBOL_WARNING "\nAlert",      "alert",      0xF44336, true  },
    { LV_SYMBOL_CLOSE   "\nDismiss",    "dismiss",    0x555566, false },
};
#define ACTION_COUNT (sizeof(k_actions) / sizeof(k_actions[0]))

static lv_obj_t * s_tile_objs[ACTION_COUNT];

// ── Text-input overlay (Note / Alert) ────────────────────────────────────
static lv_obj_t * s_overlay   = nullptr;
static lv_obj_t * s_title_lbl = nullptr;
static lv_obj_t * s_ta        = nullptr;
static lv_obj_t * s_kbd       = nullptr;
static uint8_t    s_pending_idx = 0;

static void hide_overlay(void)
{
    if (!s_overlay) return;
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_ta) lv_textarea_set_text(s_ta, "");
    uiSetSwipeEnabled(true);
}

static void submit_ev(lv_event_t * e)
{
    (void)e;
    const char * text = lv_textarea_get_text(s_ta);
    if (text && strlen(text) > 0) {
        edgeApiTriggerAction(k_actions[s_pending_idx].action_type, text);
    }
    hide_overlay();
}

static void cancel_ev(lv_event_t * e)
{
    (void)e;
    hide_overlay();
}

static void kbd_ready_ev(lv_event_t * e)
{
    (void)e;
    submit_ev(nullptr);
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
    lv_obj_set_style_pad_all(s_overlay, 8, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    s_title_lbl = lv_label_create(s_overlay);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_align(s_title_lbl, LV_ALIGN_TOP_LEFT, 4, 0);

    lv_obj_t * cancel_btn = lv_button_create(s_overlay);
    lv_obj_set_size(cancel_btn, 28, 24);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_RIGHT, 0, -2);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x2A2A3A), 0);
    lv_obj_add_event_cb(cancel_btn, cancel_ev, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * cancel_ico = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_ico, LV_SYMBOL_CLOSE);
    lv_obj_center(cancel_ico);

    s_ta = lv_textarea_create(s_overlay);
    lv_obj_set_size(s_ta, UI_SCREEN_W - 16, 40);
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 26);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_placeholder_text(s_ta, "Type here\u2026");

    lv_obj_t * send_btn = lv_button_create(s_overlay);
    lv_obj_set_size(send_btn, UI_SCREEN_W - 16, 28);
    lv_obj_align(send_btn, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_style_bg_color(send_btn, lv_color_hex(UI_CLR_ACCENT), 0);
    lv_obj_add_event_cb(send_btn, submit_ev, LV_EVENT_CLICKED, nullptr);
    lv_obj_t * send_lbl = lv_label_create(send_btn);
    lv_label_set_text(send_lbl, LV_SYMBOL_OK "  Send");
    lv_obj_center(send_lbl);

    s_kbd = lv_keyboard_create(s_overlay);
    lv_obj_set_size(s_kbd, UI_SCREEN_W - 16, UI_CAROUSEL_H - 118);
    lv_obj_align(s_kbd, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_keyboard_set_textarea(s_kbd, s_ta);
    lv_obj_add_event_cb(s_kbd, kbd_ready_ev, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(s_kbd, cancel_ev, LV_EVENT_CANCEL, nullptr);
}

static void show_overlay(uint8_t idx)
{
    s_pending_idx = idx;
    lv_label_set_text(s_title_lbl, strcmp(k_actions[idx].action_type, "note") == 0 ? "New Note" : "New Alert");
    lv_textarea_set_text(s_ta, "");
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(s_kbd, s_ta);
    uiSetSwipeEnabled(false);   // keyboard taps were being misread as carousel swipes
}

// ── Immediate-fire tiles (Time Track / Dismiss) ──────────────────────────
static void anim_opa_cb(void * obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0); }

static void play_tap_feedback(lv_obj_t * obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_40, LV_OPA_COVER);
    lv_anim_set_duration(&a, 220);
    lv_anim_start(&a);
}

static void action_clicked_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    const action_tile_t * action = &k_actions[idx];

    if (action->needs_text) {
        show_overlay((uint8_t)idx);
        return;
    }

    play_tap_feedback(s_tile_objs[idx]);
    edgeApiTriggerAction(action->action_type, "");
    Serial.printf("[Actions] Fired: %s\n", action->action_type);
}

void uiActionsScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "ACTIONS");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 14);

    const int cols = 2, rows = 2;
    const int gap = 14, margin = 30;
    const int tile_w = (UI_SCREEN_W - margin * 2 - gap) / cols;
    const int tile_h = (UI_CAROUSEL_H - 40 - gap) / rows;

    for(uint8_t i = 0; i < ACTION_COUNT; i++) {
        int col = i % cols, row = i / cols;
        int x = margin + col * (tile_w + gap);
        int y = 40 + row * (tile_h + gap);

        lv_color_t color = lv_color_hex(k_actions[i].color_hex);
        lv_obj_t * btn = lv_obj_create(tile);
        lv_obj_set_size(btn, tile_w, tile_h);
        lv_obj_set_pos(btn, x, y);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(btn, color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_mix(lv_color_black(), color, LV_OPA_20),
                                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, action_clicked_ev, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_tile_objs[i] = btn;

        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, k_actions[i].label);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }

    build_overlay(tile);
}
