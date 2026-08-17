// Project  : Jarvis Edge Node
// File     : mqtt_client.h
// Purpose  : Subscribes to backend UI-update topics — public interface
// Depends  : network_config.h

#pragma once

// Configures the PubSubClient (broker host/port from network_config.h) and
// subscribes to the Jarvis Feed / Daily Focus topics. Call once from setup(),
// after wifiManagerInit().
void mqttClientInit();

// Call once per loop() iteration once WiFi is up. Reconnects on drop and
// pumps PubSubClient's internal loop (which invokes the message callback).
void mqttClientHandle(unsigned long now);

bool mqttClientIsConnected();
