// Project  : Jarvis Edge Node
// File     : settings.h
// Purpose  : Persistent key-value settings stored on SD card — public interface
// Depends  : sd_card.h
//
// Runtime-editable WiFi/backend/MQTT config (docs/plan.txt Phase 5 — on-device
// Settings tile). Values here take priority over the compile-time defaults in
// network_config.h; those defaults are only used until the user saves settings
// for the first time via the UI.

#pragma once

// Loads /settings/jarvis.txt from SD (if present) into the in-memory mirror.
// Call after sdCardInit() in setup(). Safe to call with no SD card mounted —
// falls back to network_config.h compile-time defaults.
void settingsInit();

// Writes the in-memory mirror back to /settings/jarvis.txt on SD.
// Call after any setter below to persist across reboots.
void settingsSave();

// ── WiFi accessors ───────────────────────────────────────────────────────
const char * settingsGetWifiSSID();
void         settingsSetWifiSSID(const char * ssid);

const char * settingsGetWifiPassword();
void         settingsSetWifiPassword(const char * password);

// ── Backend accessors ────────────────────────────────────────────────────
const char * settingsGetBackendHost();
void         settingsSetBackendHost(const char * host);

int  settingsGetBackendPort();
void settingsSetBackendPort(int port);

// ── MQTT accessors ────────────────────────────────────────────────────────
const char * settingsGetMqttHost();
void         settingsSetMqttHost(const char * host);

int  settingsGetMqttPort();

void settingsSetMqttPort(int port);

// ── Pocket Recorder / Power accessors ──────────────────────────────────────
bool settingsGetAmbientVadEnabled();
void settingsSetAmbientVadEnabled(bool enabled);

bool settingsGetPowerSavingEnabled();
void settingsSetPowerSavingEnabled(bool enabled);

int  settingsGetScreenOffTimeout();
void settingsSetScreenOffTimeout(int seconds);

bool settingsGetScreenLockEnabled();
void settingsSetScreenLockEnabled(bool enabled);
