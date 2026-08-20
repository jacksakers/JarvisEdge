// Project  : House Phone
// File     : ui_screen_timers.h
// Purpose  : "Timers & Alarms" tile (docs/new_idea.txt section 4, Card 3)
// Depends  : ui.h, timers_alarms.h

#pragma once
#include <lvgl.h>

// Builds the Timers & Alarms tile's contents inside `tile`.
void uiTimersScreenInit(lv_obj_t * tile);

// Call every loop() iteration — refreshes the countdown display and flashes
// the tile while ringing.
void uiTimersScreenHandle(unsigned long now);
