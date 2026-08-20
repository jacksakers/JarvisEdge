// Project  : House Phone
// File     : ui_screen_voice.h
// Purpose  : "Jarvis Voice Capture" tile — record button + last AI confirmation
// Depends  : ui.h

#pragma once
#include <lvgl.h>

// Builds the voice capture tile's contents inside `tile` (a lv_tileview tile).
void uiVoiceScreenInit(lv_obj_t * tile);

// Updates the large confirmation label. Called from mqtt_client.cpp when an
// MQTT payload with the AI's short response arrives.
void uiVoiceSetText(const char * text);
