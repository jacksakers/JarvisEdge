// Project  : House Phone
// File     : mqtt_client.h
// Purpose  : Subscribes to backend UI-update topics — public interface
// Depends  : network_config.h

#pragma once

// Configures the PubSubClient (broker host/port from settings.h, falling back
// to network_config.h) and subscribes to the Jarvis Voice Capture feed topic.
// Call once from setup(), after wifiManagerInit().
void mqttClientInit();

// Re-reads settings and reconnects to a (possibly new) broker — call after
// the user saves new MQTT settings in the on-device Settings tile.
void mqttClientReconnect();

// Call once per loop() iteration once WiFi is up. Reconnects on drop and
// pumps PubSubClient's internal loop (which invokes the message callback).
void mqttClientHandle(unsigned long now);

bool mqttClientIsConnected();
