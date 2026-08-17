// Project  : Jarvis Edge Node
// File     : ui_screen_focus.h
// Purpose  : "Daily Focus" tile — top 3 actionable items, tap to strike through
// Depends  : ui.h

#pragma once
#include <lvgl.h>

#define UI_FOCUS_ITEM_COUNT 3

// Builds the focus tile's contents inside `tile`.
void uiFocusScreenInit(lv_obj_t * tile);

// Sets the text of one of the 3 focus slots (idx 0..UI_FOCUS_ITEM_COUNT-1).
// Local-only — clears any server-synced id, so tapping this slot won't
// call back to the backend (used for the initial placeholder text).
void uiFocusSetItem(int idx, const char * text);

// MQTT-driven update (see mqtt_client.cpp): sets both the text and the
// backend FocusItem.id for a slot. Pass id < 0 for an empty/placeholder
// slot. Tapping a slot with id >= 0 calls edgeApiToggleFocus(id) so the
// server's `done` flag stays in sync with the on-device strike-through.
void uiFocusSetItemSynced(int idx, int id, const char * text);
