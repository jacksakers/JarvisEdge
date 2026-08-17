// Project  : Jarvis Edge Node
// File     : device_heartbeat.cpp
// Purpose  : Periodic POST /device/heartbeat so the Command Center can tell
//            whether the device is actually online (docs/sdd.txt gap — the
//            device otherwise only talks to the backend when uploading a
//            queued recording or toggling a Focus item)
// Depends  : device_heartbeat.h, settings.h, wifi_manager.h, sd_card.h

#include "device_heartbeat.h"
#include "settings.h"
#include "wifi_manager.h"
#include "sd_card.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define HEARTBEAT_INTERVAL_MS  20000UL

static void send_heartbeat()
{
    char url[96];
    snprintf(url, sizeof(url), "http://%s:%d/device/heartbeat",
             settingsGetBackendHost(), settingsGetBackendPort());

    char body[128];
    snprintf(body, sizeof(body),
             "{\"wifi_rssi\":%d,\"queue_count\":%d,\"firmware\":\"jarvis-edge-1.0\"}",
             WiFi.RSSI(), sdCardCountQueueFiles());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST((uint8_t *)body, strlen(body));
    http.end();

    if (code != 200) {
        Serial.printf("[Heartbeat] POST failed: HTTP %d\n", code);
    }
}

void deviceHeartbeatInit()
{
    Serial.println("[Heartbeat] Armed — reporting every 20s while WiFi is connected.");
}

void deviceHeartbeatHandle(unsigned long now)
{
    static unsigned long s_last_sent = 0;
    if (now - s_last_sent < HEARTBEAT_INTERVAL_MS) return;
    s_last_sent = now;

    if (!wifiManagerIsConnected()) return;
    send_heartbeat();
}
