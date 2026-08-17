// Project  : Jarvis Edge Node
// File     : main.cpp
// Purpose  : Phase 1 entry point — hardware baseline + UI shell
// Depends  : display.h, ui.h
//
// Scope (docs/plan.txt Phase 1): initialise the ILI9488 panel + GT911 touch,
// bring up LVGL, and render the swipeable smartwatch-style carousel
// (Daily Focus / Jarvis Feed / Action Grid) under a persistent status bar.
// No WiFi, SD, I2S, or MQTT yet — those are later phases.

#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "ui.h"

void setup()
{
    Serial.begin(460800);
    delay(300);   // brief settle time for UART

    Serial.println();
    Serial.println("=================================================");
    Serial.println("===== Jarvis Edge Node — Phase 1 (UI Shell) =====");
    Serial.println("=================================================");
    Serial.printf("Free heap : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.flush();

    initDisplay();   // LovyanGFX panel + GT911 touch + LVGL
    ui_init();       // status bar + swipeable tile carousel

    Serial.println("[JarvisEdge] UI ready. Entering loop.");
}

void loop()
{
    lv_timer_handler();   // LVGL animations, touch handling, redraws

    // 5 ms yield keeps the LVGL timer accurate without blocking touch input.
    delay(5);
}
