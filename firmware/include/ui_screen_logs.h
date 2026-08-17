// Project  : Jarvis Edge Node
// File     : ui_screen_logs.h
// Purpose  : "Voice Logs" tile — displays voice logs and their transcriptions saved to SD card
// Depends  : ui.h

#pragma once
#include <lvgl.h>

// Builds the voice logs tile's contents inside `tile`.
void uiLogsScreenInit(lv_obj_t * tile);

// Reloads logs from `/logs/history.txt` on the SD card.
void uiLogsScreenReload();
