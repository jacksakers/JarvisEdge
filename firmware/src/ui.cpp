// Project  : Jarvis Edge Node
// File     : ui.cpp
// Purpose  : Builds the persistent status bar + swipeable tile carousel
// Depends  : ui.h, ui_status_bar.h, ui_screen_feed.h, ui_screen_focus.h, ui_screen_actions.h

#include "ui.h"
#include "ui_status_bar.h"
#include "ui_screen_feed.h"
#include "ui_screen_focus.h"
#include "ui_screen_actions.h"
#include "ui_screen_settings.h"
#include "ui_screen_logs.h"
#include "display.h"

static lv_obj_t * s_tileview     = nullptr;
static lv_obj_t * s_lock_overlay = nullptr;

static void lock_overlay_gesture_ev(lv_event_t * e)
{
    (void)e;
    lv_indev_t * indev = lv_indev_active();
    if (!indev) return;
    if (lv_indev_get_gesture_dir(indev) == LV_DIR_TOP) {
        displayForceUnlock();
    }
}

void ui_init()
{
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    uiStatusBarInit(scr);

    // Smartwatch-style horizontal carousel below the status bar.
    // Column order: Daily Focus <- Jarvis Feed (home) -> Action Grid -> Settings.
    lv_obj_t * tileview = lv_tileview_create(scr);
    s_tileview = tileview;
    lv_obj_set_size(tileview, UI_SCREEN_W, UI_CAROUSEL_H);
    lv_obj_set_pos(tileview, 0, UI_CAROUSEL_Y);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    lv_obj_set_style_pad_all(tileview, 0, 0);
    lv_obj_t * tile_focus = lv_tileview_add_tile(tileview, UI_TILE_FOCUS, 0, LV_DIR_HOR);
    lv_obj_t * tile_feed  = lv_tileview_add_tile(tileview, UI_TILE_FEED, 0, LV_DIR_HOR);
    lv_obj_t * tile_actions = lv_tileview_add_tile(tileview, UI_TILE_ACTIONS, 0, LV_DIR_HOR);
    lv_obj_t * tile_settings = lv_tileview_add_tile(tileview, UI_TILE_SETTINGS, 0, LV_DIR_HOR);
    lv_obj_t * tile_logs = lv_tileview_add_tile(tileview, UI_TILE_LOGS, 0, LV_DIR_HOR);

    uiFocusScreenInit(tile_focus);
    uiFeedScreenInit(tile_feed);
    uiActionsScreenInit(tile_actions);
    uiSettingsScreenInit(tile_settings);
    uiLogsScreenInit(tile_logs);

    // Jarvis Feed is home — open there on boot.
    lv_tileview_set_tile_by_index(tileview, UI_TILE_FEED, 0, LV_ANIM_OFF);

    // Full-screen "locked" overlay — hidden until Settings > Screen Lock is
    // enabled and the device wakes from a screen-off state (display.cpp).
    // Swipe up dismisses it; plain taps are ignored so a pocket bump alone
    // can't unlock the device.
    s_lock_overlay = lv_obj_create(scr);
    lv_obj_set_size(s_lock_overlay, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_pos(s_lock_overlay, 0, 0);
    lv_obj_remove_flag(s_lock_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_lock_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(s_lock_overlay, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(s_lock_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_lock_overlay, 0, 0);
    lv_obj_set_style_radius(s_lock_overlay, 0, 0);
    lv_obj_add_flag(s_lock_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_lock_overlay, lock_overlay_gesture_ev, LV_EVENT_GESTURE, nullptr);

    lv_obj_t * lock_lbl = lv_label_create(s_lock_overlay);
    lv_label_set_text(lock_lbl, LV_SYMBOL_CLOSE "  LOCKED");
    lv_obj_set_style_text_color(lock_lbl, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_text_font(lock_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(lock_lbl, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t * lock_hint = lv_label_create(s_lock_overlay);
    lv_label_set_text(lock_hint, LV_SYMBOL_UP "  Swipe up to unlock");
    lv_obj_set_style_text_color(lock_hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(lock_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(lock_hint, LV_ALIGN_CENTER, 0, 20);

    // Placeholder status bar state until real WiFi/LoRa/battery/queue wiring
    // lands in later phases (see docs/plan.txt Phase 2-4).
    uiStatusBarSetWifiConnected(false);
    uiStatusBarSetLoraConnected(false);
    uiStatusBarSetBatteryPercent(-1);
    uiStatusBarSetQueueCount(0);
}

void uiSetSwipeEnabled(bool enabled)
{
    if (!s_tileview) return;
    lv_obj_set_scroll_dir(s_tileview, enabled ? LV_DIR_HOR : LV_DIR_NONE);
}

void uiSetLockOverlayVisible(bool visible)
{
    if (!s_lock_overlay) return;
    if (visible) {
        lv_obj_remove_flag(s_lock_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_lock_overlay);
    } else {
        lv_obj_add_flag(s_lock_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
