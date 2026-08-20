// Project  : House Phone
// File     : main.cpp
// Purpose  : Entry point — hardware baseline, UI shell, offline audio queue,
//            WiFi/MQTT sync with the home server backend, on-device settings,
//            Ambient Home (Tapo), Landline Feed (BLE), and Timers & Alarms
// Depends  : display.h, ui.h, sd_card.h, settings.h, plaud_mode.h,
//            wifi_manager.h, mqtt_client.h, sync_manager.h, tapo_control.h,
//            ble_notifications.h, timers_alarms.h
//
// See docs/sdd.txt for the full system design and docs/new_idea.txt for the
// original House Phone pivot proposal.

#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "ui.h"
#include "ui_status_bar.h"
#include "ui_screen_landline.h"
#include "ui_screen_timers.h"
#include "sd_card.h"
#include "settings.h"
#include "plaud_mode.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "sync_manager.h"
#include "device_heartbeat.h"
#include "tapo_control.h"
#include "ble_notifications.h"
#include "timers_alarms.h"

void setup()
{
    Serial.begin(460800);
    delay(300);   // brief settle time for UART

    Serial.println();
    Serial.println("=================================================");
    Serial.println("==            House Phone — booting            ==");
    Serial.println("=================================================");
    Serial.printf("Free heap : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.flush();

    initDisplay();   // LovyanGFX panel + GT911 touch + LVGL
    sdCardInit();        // mount /queue for offline recordings
    settingsInit();      // load WiFi/backend/MQTT/alarm config from SD (Settings tile)

    if (settingsGetPowerSavingEnabled()) {
        setCpuFrequencyMhz(80);
        Serial.println("[Power] Power saving mode enabled - Scaling CPU to 80MHz.");
    }

    ui_init();       // status bar + swipeable tile carousel (reads settings)

    plaudModeInit();     // arm BOOT button + mic capture writer task
    uiStatusBarSetQueueCount(sdCardCountQueueFiles());

    wifiManagerInit();   // background WiFi connect (non-blocking)
    mqttClientInit();    // configure broker + UI-update subscription
    syncManagerInit();   // background auto-sync task (core 0)
    deviceHeartbeatInit(); // periodic online/offline ping to the Command Center
    tapoControlInit();     // Ambient Home backend client
    bleNotificationsInit(); // Landline Feed BLE server (Tasker bridge)
    timersAlarmsInit();     // cooking timer + bedside alarm buzzer

    Serial.println("[House Phone] Init complete. Entering loop.");
}

void loop()
{
    // Drives LVGL when idle, or mic capture while Plaud mode is recording —
    // never both, per docs/sdd.txt section 4.1.
    plaudModeHandle();

    // Lightweight, non-blocking — safe to poll every tick regardless of
    // Plaud mode (auto-sync itself runs on its own core-0 task).
    unsigned long now = millis();
    wifiManagerHandle(now);
    mqttClientHandle(now);
    deviceHeartbeatHandle(now);
    displayHandle(now);
    tapoControlHandle(now);
    uiLandlineScreenHandle();
    timersAlarmsHandle(now);
    uiTimersScreenHandle(now);

    // 5 ms yield keeps timing accurate without blocking touch/I2S.
    delay(5);
}
