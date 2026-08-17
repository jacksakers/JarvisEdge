// Project  : Jarvis Edge Node
// File     : ui_screen_feed.h
// Purpose  : "Jarvis Feed" home tile — most recent AI confirmation/alert
// Depends  : ui.h

#pragma once
#include <lvgl.h>

// Builds the feed tile's contents inside `tile` (a lv_tileview tile object).
void uiFeedScreenInit(lv_obj_t * tile);

// Updates the large feed label. Called later (Phase 4) when an MQTT
// payload with the AI's short response arrives.
void uiFeedSetText(const char * text);
