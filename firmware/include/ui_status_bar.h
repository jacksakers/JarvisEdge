// Project  : House Phone
// File     : ui_status_bar.h
// Purpose  : Persistent top status bar — battery, WiFi, LoRa, queue indicators
// Depends  : ui.h
//
// Phase 1 only builds the bar and exposes setters; nothing calls them yet.
// Later phases will wire these to real WiFi/battery/LoRa/queue state.

#pragma once
#include <lvgl.h>

// Builds the status bar as a full-width object pinned to the top of `parent`.
void uiStatusBarInit(lv_obj_t * parent);

void uiStatusBarSetWifiConnected(bool connected);
void uiStatusBarSetLoraConnected(bool connected);
void uiStatusBarSetBatteryPercent(int percent);   // -1 = unknown/hide
void uiStatusBarSetQueueCount(int count);         // 0 hides the badge
