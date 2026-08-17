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
// Called later (Phase 4) when the server pushes an updated Daily Focus list.
void uiFocusSetItem(int idx, const char * text);
