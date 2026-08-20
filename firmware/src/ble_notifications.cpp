// Project  : House Phone
// File     : ble_notifications.cpp
// Purpose  : BLE GATT server — receives Tasker-relayed phone notifications
// Depends  : ble_notifications.h, BLEDevice (bundled with arduino-esp32)

#include "ble_notifications.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <time.h>
#include <string>

// Randomly generated UUIDs — arbitrary but fixed so the Tasker profile only
// has to be configured once. Override via build_flags if you run more than
// one House Phone on the same network and Tasker needs to disambiguate.
#define BLE_SERVICE_UUID         "7b1e6d20-7b2a-4a7a-9b0e-9f6f7f6a2a10"
#define BLE_NOTIF_CHAR_UUID      "7b1e6d21-7b2a-4a7a-9b0e-9f6f7f6a2a10"

static BleNotification s_ring[BLE_NOTIF_MAX];
static int             s_count = 0;   // number of valid entries, s_ring[0] = newest
static volatile bool   s_update_flag = false;

static void push_notification(const char * app, const char * title, const char * text, unsigned long ts)
{
    // Shift everything down (drop oldest if full), then write the newest at [0].
    int last = (s_count < BLE_NOTIF_MAX) ? s_count : BLE_NOTIF_MAX - 1;
    for (int i = last; i > 0; i--) s_ring[i] = s_ring[i - 1];

    BleNotification & n = s_ring[0];
    strncpy(n.app, app ? app : "", sizeof(n.app) - 1); n.app[sizeof(n.app) - 1] = '\0';
    strncpy(n.title, title ? title : "", sizeof(n.title) - 1); n.title[sizeof(n.title) - 1] = '\0';
    strncpy(n.text, text ? text : "", sizeof(n.text) - 1); n.text[sizeof(n.text) - 1] = '\0';
    n.ts = ts;

    if (s_count < BLE_NOTIF_MAX) s_count++;
    s_update_flag = true;
}

class NotificationWriteCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic * characteristic) override
    {
        std::string value = characteristic->getValue();
        if (value.length() == 0) return;

        JsonDocument doc;
        if (deserializeJson(doc, value.c_str(), value.length()) != DeserializationError::Ok) {
            Serial.println("[BLE] Notification write was not valid JSON — dropped.");
            return;
        }

        unsigned long ts = doc["ts"] | 0;
        if (ts == 0) ts = time(nullptr) > 100000 ? (unsigned long)time(nullptr) : millis() / 1000;

        push_notification(doc["app"] | "", doc["title"] | "(no title)", doc["text"] | "", ts);
        Serial.printf("[BLE] Notification received: %s / %s\n",
                      (const char *)(doc["app"] | ""), (const char *)(doc["title"] | ""));
    }
};

class ServerConnectCallback : public BLEServerCallbacks {
    void onConnect(BLEServer * server) override
    {
        Serial.println("[BLE] Tasker bridge connected.");
    }
    void onDisconnect(BLEServer * server) override
    {
        Serial.println("[BLE] Tasker bridge disconnected — resuming advertising.");
        BLEDevice::startAdvertising();
    }
};

void bleNotificationsInit()
{
    BLEDevice::init("House Phone");
    BLEServer * server = BLEDevice::createServer();
    server->setCallbacks(new ServerConnectCallback());

    BLEService * service = server->createService(BLE_SERVICE_UUID);
    BLECharacteristic * notif_char = service->createCharacteristic(
        BLE_NOTIF_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    notif_char->setCallbacks(new NotificationWriteCallback());
    notif_char->addDescriptor(new BLE2902());

    service->start();

    BLEAdvertising * advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising as \"House Phone\" — Landline Feed ready for Tasker.");
}

bool bleNotificationsConsumeUpdate()
{
    if (!s_update_flag) return false;
    s_update_flag = false;
    return true;
}

int bleNotificationsGetCount()
{
    return s_count;
}

const BleNotification * bleNotificationsGet(int idx)
{
    if (idx < 0 || idx >= s_count) return nullptr;
    return &s_ring[idx];
}

void bleNotificationsDismiss(int idx)
{
    if (idx < 0 || idx >= s_count) return;
    for (int i = idx; i < s_count - 1; i++) s_ring[i] = s_ring[i + 1];
    s_count--;
}

void bleNotificationsClear()
{
    s_count = 0;
}
