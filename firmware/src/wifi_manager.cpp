// Project  : Jarvis Edge Node
// File     : wifi_manager.cpp
// Purpose  : Non-blocking WiFi connection + status bar wiring
// Depends  : wifi_manager.h, network_config.h, ui_status_bar.h

#include "wifi_manager.h"
#include "network_config.h"
#include "ui_status_bar.h"
#include <Arduino.h>
#include <WiFi.h>

static bool s_was_connected = false;

void wifiManagerInit()
{
    if (strlen(JARVIS_WIFI_SSID) == 0) {
        Serial.println("[WiFi] No SSID configured — skipping. Set JARVIS_WIFI_SSID in network_config.h.");
        return;
    }

    Serial.printf("[WiFi] Starting background connection to: %s\n", JARVIS_WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(JARVIS_WIFI_SSID, JARVIS_WIFI_PASSWORD);
}

void wifiManagerHandle(unsigned long now)
{
    static unsigned long s_last_check = 0;
    if (now - s_last_check < 1000UL) return;
    s_last_check = now;

    bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected == s_was_connected) return;

    s_was_connected = connected;
    uiStatusBarSetWifiConnected(connected);

    if (connected) {
        Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WiFi] Disconnected.");
    }
}

bool wifiManagerIsConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
