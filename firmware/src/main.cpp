// Project  : Jarvis Edge Node
// File     : main.cpp
// Purpose  : Phase 1+2 entry point — hardware baseline, UI shell, offline audio queue
// Depends  : display.h, ui.h, sd_card.h, plaud_mode.h
//
// Scope (docs/plan.txt):
//   Phase 1 — ILI9488 panel + GT911 touch, LVGL UI shell (carousel + status bar)
//   Phase 2 — SD-backed /queue, I2S mic capture, BOOT-button "Plaud mode"
// No WiFi/MQTT yet — that's Phase 3+.

#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "ui.h"
#include "ui_status_bar.h"
#include "sd_card.h"
#include "plaud_mode.h"

void setup()
{
    Serial.begin(460800);
    delay(300);   // brief settle time for UART

    Serial.println();
    Serial.println("=================================================");
    Serial.println("== Jarvis Edge Node — Phase 2 (Offline Capture) ==");
    Serial.println("=================================================");
    Serial.printf("Free heap : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.flush();

    initDisplay();   // LovyanGFX panel + GT911 touch + LVGL
    ui_init();       // status bar + swipeable tile carousel

    sdCardInit();        // mount /queue for offline recordings
    plaudModeInit();     // arm BOOT button + mic capture writer task
    uiStatusBarSetQueueCount(sdCardCountQueueFiles());

    Serial.println("[JarvisEdge] Init complete. Entering loop.");
}

void loop()
{
    // Drives LVGL when idle, or mic capture while Plaud mode is recording —
    // never both, per docs/sdd.txt section 4.1.
    plaudModeHandle();

    // 5 ms yield keeps timing accurate without blocking touch/I2S.
    delay(5);
}
