// Project  : House Phone
// File     : device_heartbeat.h
// Purpose  : Periodic POST /device/heartbeat so the Command Center can tell
//            whether the device is actually online — public interface
// Depends  : (none)

#pragma once

// Call once from setup(), after settingsInit().
void deviceHeartbeatInit();

// Call once per loop() iteration alongside wifiManagerHandle/mqttClientHandle.
// Non-blocking — only fires an HTTP POST every HEARTBEAT_INTERVAL_MS.
void deviceHeartbeatHandle(unsigned long now);
