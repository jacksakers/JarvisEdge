// Project  : Jarvis Edge Node
// File     : ui_screen_actions.cpp
// Purpose  : "Action Grid" tile — 2x2 grid of manual trigger buttons
// Depends  : ui_screen_actions.h
//
// Phase 1 wires up the grid visually only; the callbacks just log to
// Serial. Real behaviour (audio capture, alerts, MQTT publish) lands in
// later phases per docs/plan.txt.

#include "ui_screen_actions.h"
#include "ui.h"
#include <Arduino.h>

typedef struct {
    const char * label;
    uint32_t     color_hex;
} action_tile_t;

static const action_tile_t k_actions[] = {
    { LV_SYMBOL_CALL   "\nTime Track", 0x2196F3 },
    { LV_SYMBOL_EDIT   "\nNote",       0x4CAF50 },
    { LV_SYMBOL_WARNING "\nAlert",     0xF44336 },
    { LV_SYMBOL_CLOSE  "\nDismiss",    0x555566 },
};
#define ACTION_COUNT (sizeof(k_actions) / sizeof(k_actions[0]))

static void action_clicked_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("[Actions] Tapped: %s\n", k_actions[idx].label);
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

        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, k_actions[i].label);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }
}
