// Project  : Jarvis Edge Node
// File     : wifi_manager.h
// Purpose  : Non-blocking WiFi connection — public interface
// Depends  : network_config.h

#pragma once

// Starts a background connection attempt using the SSID/password from
// settings.h (falls back to network_config.h compile-time defaults).
// Safe to call with no SSID configured (logs and does nothing).
void wifiManagerInit();

// Re-reads settings and reconnects — call after the user saves new WiFi
// credentials in the on-device Settings tile.
void wifiManagerReconnect();

// Call once per loop() iteration. Logs connect/disconnect transitions and
// updates the status bar's WiFi icon.
void wifiManagerHandle(unsigned long now);

bool wifiManagerIsConnected();
