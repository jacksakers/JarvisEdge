// Project  : House Phone
// File     : tapo_control.h
// Purpose  : Ambient Home tile's backend client — Tapo bulb zones are polled
//            and controlled via the FastAPI backend's /tapo/* endpoints
//            (see ../../backend/app/tapo.py), never via KLAP directly from
//            the ESP32 (docs/coding.txt 2.1 — keep the edge device "dumb").
// Depends  : settings.h

#pragma once

#define TAPO_MAX_ZONES  6

struct TapoZoneState {
    int  id;                 // backend TapoZone.id, -1 = empty slot
    char name[32];
    char room[24];
    bool on;
    int  brightness;         // 1-100
    bool reachable;
};

// Arms periodic polling. Call once from setup().
void tapoControlInit();

// Call every loop() iteration — polls GET /tapo/zones every few seconds
// (blocking HTTPClient call, same pattern as device_heartbeat.cpp) and
// refreshes the Ambient Home grid via uiHomeSetZones() on success.
void tapoControlHandle(unsigned long now);

// Fire-and-forget POSTs (Core-0 FreeRTOS task, mirrors the old edge_api.cpp
// pattern) so a slow/unreachable backend never blocks LVGL taps. The next
// tapoControlHandle() poll reconciles the on-device grid with reality.
void tapoControlToggle(int zone_id);
void tapoControlSetBrightness(int zone_id, int brightness);
void tapoControlAllOff();
