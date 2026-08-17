// Project  : Jarvis Edge Node
// File     : wifi_manager.cpp
// Purpose  : Non-blocking WiFi connection + status bar wiring
// Depends  : wifi_manager.h, network_config.h, ui_status_bar.h

#include "wifi_manager.h"
#include "network_config.h"
#include "settings.h"
#include "ui_status_bar.h"
#include <Arduino.h>
#include <WiFi.h>

static bool s_was_connected = false;

void wifiManagerInit()
{
    const char * ssid = settingsGetWifiSSID();
    const char * pass = settingsGetWifiPassword();
    if (!ssid || ssid[0] == '\0') {
        Serial.println("[WiFi] No SSID configured — skipping. Set it via the on-device Settings tile.");
        return;
    }

    Serial.printf("[WiFi] Starting background connection to: %s\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
}

void wifiManagerReconnect()
{
    WiFi.disconnect(false);
    wifiManagerInit();
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
        configTzTime("EST5EDT,M3.2.0,M11.1.0", "pool.ntp.org", "time.nist.gov");
    } else {
        Serial.println("[WiFi] Disconnected.");
    }
}

bool wifiManagerIsConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
