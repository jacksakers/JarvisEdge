// Project  : House Phone
// File     : ble_notifications.h
// Purpose  : BLE GATT server that receives phone notifications relayed by
//            Tasker (docs/new_idea.txt section 3.2 — "The Mobile Bridge"),
//            feeding the Landline Feed tile — public interface
// Depends  : (none)
//
// Tasker profile setup (companion "Mobile Bridge"): use a BLE/GATT plugin
// (e.g. "AutoNotification" + a BLE serial/GATT writer plugin) to connect to
// this device (advertised name "House Phone") and write a UTF-8 JSON string
// to the notification characteristic below whenever an Android notification
// posts. Expected JSON schema:
//
//   {"app": "Messages", "title": "Jane Doe", "text": "On my way!", "ts": 1699999999}
//
// - "app"   : short source label shown as a pill (e.g. app name). Optional.
// - "title" : bold headline (e.g. contact name / SMS sender). Required.
// - "text"  : body text. Optional.
// - "ts"    : unix epoch seconds. Optional — falls back to millis()/1000 if
//             omitted (so the on-device list still sorts sanely without NTP).
//
// A message longer than the fixed buffers below is truncated, not rejected.

#pragma once
#include <stdint.h>

#define BLE_NOTIF_MAX      8   // ring buffer capacity — oldest is dropped first
#define BLE_NOTIF_APP_LEN  24
#define BLE_NOTIF_TITLE_LEN 40
#define BLE_NOTIF_TEXT_LEN  96

struct BleNotification {
    char app[BLE_NOTIF_APP_LEN];
    char title[BLE_NOTIF_TITLE_LEN];
    char text[BLE_NOTIF_TEXT_LEN];
    unsigned long ts;   // unix epoch seconds, best-effort
};

// Starts the BLE server + advertising ("House Phone"). Call once from setup().
void bleNotificationsInit();

// Returns true (and clears the flag) exactly once per new notification —
// call every loop() iteration from ui_screen_landline.cpp to know when to
// refresh the card list. BLE write callbacks run on the Bluedroid host task,
// not the main loop, so this hand-off keeps all LVGL calls on one thread.
bool bleNotificationsConsumeUpdate();

// Most-recent-first accessors. idx 0 is the newest notification.
int bleNotificationsGetCount();
const BleNotification * bleNotificationsGet(int idx);

// Removes one notification (tap-to-dismiss) or clears the whole list.
void bleNotificationsDismiss(int idx);
void bleNotificationsClear();
