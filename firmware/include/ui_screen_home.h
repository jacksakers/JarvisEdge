// Project  : House Phone
// File     : ui_screen_home.h
// Purpose  : "Ambient Home" tile — grid of Tapo bulb zones (docs/new_idea.txt
//            section 4, Card 1). Tap toggles on/off, long-press opens a
//            brightness slider, and an "All Off" button clears the room.
// Depends  : ui.h, tapo_control.h

#pragma once
#include <lvgl.h>
#include "tapo_control.h"

// Builds the Ambient Home tile's contents inside `tile`.
void uiHomeScreenInit(lv_obj_t * tile);

// Called by tapo_control.cpp after each successful /tapo/zones poll —
// rebuilds tile labels/colors from the current zone list (up to TAPO_MAX_ZONES).
void uiHomeSetZones(const TapoZoneState * zones, int count);
