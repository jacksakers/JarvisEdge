// Project  : House Phone
// File     : ui_screen_landline.cpp
// Purpose  : "Landline Feed" tile — scrollable list of BLE notification cards
// Depends  : ui_screen_landline.h, ui.h, ble_notifications.h

#include "ui_screen_landline.h"
#include "ui.h"
#include "ble_notifications.h"
#include <Arduino.h>

static lv_obj_t * s_list       = nullptr;
static lv_obj_t * s_empty_hint = nullptr;
static lv_obj_t * s_cards[BLE_NOTIF_MAX];
static lv_obj_t * s_title_labels[BLE_NOTIF_MAX];
static lv_obj_t * s_text_labels[BLE_NOTIF_MAX];

static void anim_opa_cb(void * obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0); }

static void refresh_from_state()
{
    int count = bleNotificationsGetCount();

    if (s_empty_hint) {
        if (count == 0) lv_obj_remove_flag(s_empty_hint, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_empty_hint, LV_OBJ_FLAG_HIDDEN);
    }

    for (int i = 0; i < BLE_NOTIF_MAX; i++) {
        const BleNotification * n = bleNotificationsGet(i);
        if (!n) {
            lv_obj_add_flag(s_cards[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        char header[BLE_NOTIF_APP_LEN + BLE_NOTIF_TITLE_LEN + 4];
        if (n->app[0]) {
            snprintf(header, sizeof(header), "%s \xC2\xB7 %s", n->app, n->title);
        } else {
            strncpy(header, n->title, sizeof(header) - 1);
            header[sizeof(header) - 1] = '\0';
        }
        lv_label_set_text(s_title_labels[i], header);
        lv_label_set_text(s_text_labels[i], n->text);
        lv_obj_remove_flag(s_cards[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void dismiss_card_ev(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    bleNotificationsDismiss(idx);
    refresh_from_state();
}

static void card_clicked_ev(lv_event_t * e)
{
    // Tap anywhere on the card flashes + dismisses it — same gesture as
    // reading and swiping away a phone notification.
    lv_obj_t * card = (lv_obj_t *)lv_event_get_target(e);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_40, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 150);
    lv_anim_set_completed_cb(&a, [](lv_anim_t * anim) {
        lv_obj_set_style_opa((lv_obj_t *)anim->var, LV_OPA_COVER, 0);
    });
    lv_anim_start(&a);
    dismiss_card_ev(e);
}

void uiLandlineScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "LANDLINE FEED");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 6);

    s_empty_hint = lv_label_create(tile);
    lv_label_set_text(s_empty_hint, "No notifications yet.\nPair your phone's Tasker bridge to the\n\"House Phone\" BLE device.");
    lv_obj_set_style_text_align(s_empty_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_empty_hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(s_empty_hint, &lv_font_montserrat_14, 0);
    lv_obj_center(s_empty_hint);

    s_list = lv_obj_create(tile);
    lv_obj_set_size(s_list, UI_SCREEN_W - 16, UI_CAROUSEL_H - 30);
    lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_pad_gap(s_list, 6, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);

    for (int i = 0; i < BLE_NOTIF_MAX; i++) {
        lv_obj_t * card = lv_obj_create(s_list);
        lv_obj_set_size(card, UI_SCREEN_W - 16, LV_SIZE_CONTENT);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(UI_CLR_SURFACE), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(card, card_clicked_ev, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_cards[i] = card;

        lv_obj_t * title_lbl = lv_label_create(card);
        lv_obj_set_width(title_lbl, UI_SCREEN_W - 40);
        lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title_lbl, lv_color_hex(UI_CLR_ACCENT), 0);
        s_title_labels[i] = title_lbl;

        lv_obj_t * text_lbl = lv_label_create(card);
        lv_obj_set_width(text_lbl, UI_SCREEN_W - 40);
        lv_label_set_long_mode(text_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(text_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(text_lbl, lv_color_hex(UI_CLR_TEXT), 0);
        s_text_labels[i] = text_lbl;
    }

    refresh_from_state();
}

void uiLandlineScreenHandle()
{
    if (bleNotificationsConsumeUpdate()) {
        refresh_from_state();
    }
}
