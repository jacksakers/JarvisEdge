// Project  : Jarvis Edge Node
// File     : ui.cpp
// Purpose  : Builds the persistent status bar + swipeable tile carousel
// Depends  : ui.h, ui_status_bar.h, ui_screen_feed.h, ui_screen_focus.h, ui_screen_actions.h

#include "ui.h"
#include "ui_status_bar.h"
#include "ui_screen_feed.h"
#include "ui_screen_focus.h"
#include "ui_screen_actions.h"

void ui_init()
{
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    uiStatusBarInit(scr);

    // Smartwatch-style horizontal carousel below the status bar.
    // Column order: Daily Focus <- Jarvis Feed (home) -> Action Grid.
    lv_obj_t * tileview = lv_tileview_create(scr);
    lv_obj_set_size(tileview, UI_SCREEN_W, UI_CAROUSEL_H);
    lv_obj_set_pos(tileview, 0, UI_CAROUSEL_Y);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    lv_obj_set_style_pad_all(tileview, 0, 0);

    lv_obj_t * tile_focus = lv_tileview_add_tile(tileview, UI_TILE_FOCUS, 0, LV_DIR_HOR);
    lv_obj_t * tile_feed  = lv_tileview_add_tile(tileview, UI_TILE_FEED, 0, LV_DIR_HOR);
    lv_obj_t * tile_actions = lv_tileview_add_tile(tileview, UI_TILE_ACTIONS, 0, LV_DIR_HOR);

    uiFocusScreenInit(tile_focus);
    uiFeedScreenInit(tile_feed);
    uiActionsScreenInit(tile_actions);

    // Jarvis Feed is home — open there on boot.
    lv_tileview_set_tile_by_index(tileview, UI_TILE_FEED, 0, LV_ANIM_OFF);

    // Placeholder status bar state until real WiFi/LoRa/battery/queue wiring
    // lands in later phases (see docs/plan.txt Phase 2-4).
    uiStatusBarSetWifiConnected(false);
    uiStatusBarSetLoraConnected(false);
    uiStatusBarSetBatteryPercent(-1);
    uiStatusBarSetQueueCount(0);
}
