// Project  : House Phone
// File     : ui_screen_landline.h
// Purpose  : "Landline Feed" tile — BLE-relayed phone notification cards
// Depends  : ui.h, ble_notifications.h

#pragma once
#include <lvgl.h>

// Builds the Landline Feed tile's contents inside `tile`.
void uiLandlineScreenInit(lv_obj_t * tile);

// Call every loop() iteration — cheap no-op unless a new BLE notification
// arrived since the last call (see bleNotificationsConsumeUpdate()).
void uiLandlineScreenHandle();
