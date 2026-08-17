// Project  : Jarvis Edge Node
// File     : ui_screen_focus.cpp
// Purpose  : "Daily Focus" tile — top 3 actionable items, tap to strike through
// Depends  : ui_screen_focus.h

#include "ui_screen_focus.h"
#include "ui.h"
#include <string.h>

static lv_obj_t * s_item_labels[UI_FOCUS_ITEM_COUNT];
static char        s_item_text[UI_FOCUS_ITEM_COUNT][64];
static bool        s_item_done[UI_FOCUS_ITEM_COUNT];

// Re-applies text + strike-through style for one slot from local state.
static void refresh_item(int idx)
{
    lv_obj_t * lbl = s_item_labels[idx];
    lv_label_set_text(lbl, s_item_text[idx]);
    if(s_item_done[idx]) {
        lv_obj_set_style_text_decor(lbl, LV_TEXT_DECOR_STRIKETHROUGH, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(UI_CLR_MUTED), 0);
    } else {
        lv_obj_set_style_text_decor(lbl, LV_TEXT_DECOR_NONE, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(UI_CLR_TEXT), 0);
    }
}

static void item_clicked_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    // Phase 1: local-only toggle. Later phases will also notify the server.
    s_item_done[idx] = !s_item_done[idx];
    refresh_item(idx);
}

void uiFocusScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "DAILY FOCUS");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 14);

    static const char * placeholders[UI_FOCUS_ITEM_COUNT] = {
        "Waiting for server sync\u2026",
        "\u2014",
        "\u2014",
    };

    for(int i = 0; i < UI_FOCUS_ITEM_COUNT; i++) {
        strncpy(s_item_text[i], placeholders[i], sizeof(s_item_text[i]) - 1);
        s_item_text[i][sizeof(s_item_text[i]) - 1] = '\0';
        s_item_done[i] = false;

        lv_obj_t * row = lv_obj_create(tile);
        lv_obj_set_size(row, UI_SCREEN_W - 40, 60);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 40 + i * 68);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_CLR_SURFACE), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_add_event_cb(row, item_clicked_ev, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t * lbl = lv_label_create(row);
        lv_obj_set_width(lbl, UI_SCREEN_W - 72);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
        s_item_labels[i] = lbl;
        refresh_item(i);
    }
}

void uiFocusSetItem(int idx, const char * text)
{
    if(idx < 0 || idx >= UI_FOCUS_ITEM_COUNT || !text) return;
    strncpy(s_item_text[idx], text, sizeof(s_item_text[idx]) - 1);
    s_item_text[idx][sizeof(s_item_text[idx]) - 1] = '\0';
    s_item_done[idx] = false;
    if(s_item_labels[idx]) refresh_item(idx);
}
