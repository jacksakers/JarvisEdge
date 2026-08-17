// Project  : Jarvis Edge Node
// File     : main.cpp
// Purpose  : Phase 1+2+4 entry point — hardware baseline, UI shell, offline
//            audio queue, WiFi/MQTT sync with the home server backend
// Depends  : display.h, ui.h, sd_card.h, plaud_mode.h, wifi_manager.h,
//            mqtt_client.h, sync_manager.h
//
// Scope (docs/plan.txt):
//   Phase 1 — ILI9488 panel + GT911 touch, LVGL UI shell (carousel + status bar)
//   Phase 2 — SD-backed /queue, I2S mic capture, BOOT-button "Plaud mode"
//   Phase 4 — WiFi + MQTT UI push-back, background auto-sync of /queue to the
//             Phase 3 backend (Phase 3 itself lives in ../../backend)

#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "ui.h"
#include "ui_status_bar.h"
#include "sd_card.h"
#include "plaud_mode.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "sync_manager.h"

void setup()
{
    Serial.begin(460800);
    delay(300);   // brief settle time for UART

    Serial.println();
    Serial.println("=================================================");
    Serial.println("== Jarvis Edge Node — Phase 4 (WiFi/MQTT Sync)  ==");
    Serial.println("=================================================");
    Serial.printf("Free heap : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.flush();

    initDisplay();   // LovyanGFX panel + GT911 touch + LVGL
    ui_init();       // status bar + swipeable tile carousel

    sdCardInit();        // mount /queue for offline recordings
    plaudModeInit();     // arm BOOT button + mic capture writer task
    uiStatusBarSetQueueCount(sdCardCountQueueFiles());

    wifiManagerInit();   // background WiFi connect (non-blocking)
    mqttClientInit();    // configure broker + UI-update subscriptions
    syncManagerInit();   // background auto-sync task (core 0)

    Serial.println("[JarvisEdge] Init complete. Entering loop.");
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

    // 5 ms yield keeps timing accurate without blocking touch/I2S.
    delay(5);
}
