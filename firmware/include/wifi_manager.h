// Project  : Jarvis Edge Node
// File     : wifi_manager.h
// Purpose  : Non-blocking WiFi connection — public interface
// Depends  : network_config.h

#pragma once

// Starts a background connection attempt using JARVIS_WIFI_SSID/PASSWORD.
// Safe to call with no SSID configured (logs and does nothing).
void wifiManagerInit();

// Call once per loop() iteration. Logs connect/disconnect transitions and
// updates the status bar's WiFi icon.
void wifiManagerHandle(unsigned long now);

bool wifiManagerIsConnected();
