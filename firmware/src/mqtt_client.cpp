// Project  : Jarvis Edge Node
// File     : mqtt_client.cpp
// Purpose  : PubSubClient wrapper — subscribes to backend UI-update topics
//            and pushes payloads into the LVGL Jarvis Feed / Daily Focus tiles
// Depends  : mqtt_client.h, network_config.h, ui_screen_feed.h, ui_screen_focus.h,
//            PubSubClient, ArduinoJson
//
// Payload schemas published by the backend (see ../../backend/app/main.py):
//   jarvis/ui/feed  : {"text": "..."}
//   jarvis/ui/focus : {"tasks": ["...", "...", "..."]}

#include "mqtt_client.h"
#include "network_config.h"
#include "settings.h"
#include "ui_screen_feed.h"
#include "ui_screen_focus.h"
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
    uiFeedSetText(doc["text"] | "");
}

static void handle_focus_payload(const uint8_t * payload, unsigned int len)
{
    JsonDocument doc;
    if (deserializeJson(doc, payload, len) != DeserializationError::Ok) return;

    // Payload shape: {"tasks": [{"id": 3, "text": "..."}, ...]}
    // (see _publish_focus_from_db() in ../../backend/app/main.py)
    JsonArrayConst tasks = doc["tasks"].as<JsonArrayConst>();
    for (int i = 0; i < UI_FOCUS_ITEM_COUNT; i++) {
        if (i < (int)tasks.size()) {
            int id = tasks[i]["id"] | -1;
            uiFocusSetItemSynced(i, id, tasks[i]["text"] | "");
        } else {
            uiFocusSetItemSynced(i, -1, "\u2014");
        }
    }
}

static void mqtt_callback(char * topic, uint8_t * payload, unsigned int len)
{
    if (strcmp(topic, JARVIS_MQTT_TOPIC_FEED) == 0) {
        handle_feed_payload(payload, len);
    } else if (strcmp(topic, JARVIS_MQTT_TOPIC_FOCUS) == 0) {
        handle_focus_payload(payload, len);
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
            s_mqtt.subscribe(JARVIS_MQTT_TOPIC_FOCUS);
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
