// Project  : House Phone
// File     : mqtt_client.cpp
// Purpose  : PubSubClient wrapper — subscribes to the backend's UI-update
//            topic and pushes payloads into the LVGL Voice Capture tile
// Depends  : mqtt_client.h, network_config.h, ui_screen_voice.h,
//            PubSubClient, ArduinoJson
//
// Payload schema published by the backend (see ../../backend/app/main.py):
//   jarvis/ui/feed : {"text": "..."}

#include "mqtt_client.h"
#include "network_config.h"
#include "settings.h"
#include "ui_screen_voice.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MQTT_RECONNECT_MS  5000UL

static WiFiClient   s_wifi_client;
static PubSubClient s_mqtt(s_wifi_client);

static void handle_feed_payload(const uint8_t * payload, unsigned int len)
{
    JsonDocument doc;
    if (deserializeJson(doc, payload, len) != DeserializationError::Ok) return;
    uiVoiceSetText(doc["text"] | "");
}

static void mqtt_callback(char * topic, uint8_t * payload, unsigned int len)
{
    if (strcmp(topic, JARVIS_MQTT_TOPIC_FEED) == 0) {
        handle_feed_payload(payload, len);
    }
}

void mqttClientInit()
{
    s_mqtt.setServer(settingsGetMqttHost(), settingsGetMqttPort());
    s_mqtt.setCallback(mqtt_callback);
    Serial.printf("[MQTT] Configured for %s:%d\n", settingsGetMqttHost(), settingsGetMqttPort());
}

void mqttClientReconnect()
{
    if (s_mqtt.connected()) s_mqtt.disconnect();
    mqttClientInit();
}

void mqttClientHandle(unsigned long now)
{
    if (WiFi.status() != WL_CONNECTED) return;

    if (!s_mqtt.connected()) {
        static unsigned long s_last_attempt = 0;
        if (now - s_last_attempt < MQTT_RECONNECT_MS) return;
        s_last_attempt = now;

        Serial.println("[MQTT] Connecting to broker...");
        if (s_mqtt.connect("jarvis-edge-node")) {
            s_mqtt.subscribe(JARVIS_MQTT_TOPIC_FEED);
            Serial.println("[MQTT] Connected and subscribed.");
        } else {
            Serial.printf("[MQTT] Connect failed, rc=%d\n", s_mqtt.state());
        }
        return;
    }

    s_mqtt.loop();
}

bool mqttClientIsConnected()
{
    return s_mqtt.connected();
}
