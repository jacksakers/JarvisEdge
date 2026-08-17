// Project  : Jarvis Edge Node
// File     : plaud_mode.cpp
// Purpose  : "Plaud mode" state machine — BOOT button toggles screen-off audio capture
// Depends  : plaud_mode.h

#include "plaud_mode.h"
#include "boot_button.h"
#include "mic_capture.h"
#include "sd_card.h"
#include "display.h"
#include "ui_status_bar.h"
#include <lvgl.h>
#include <Arduino.h>

static bool s_active = false;

static void start_recording()
{
    displaySetBacklight(false);   // kill the backlight before touching SD/I2S

    if (!micCaptureStart()) {
        displaySetBacklight(true);
        return;
    }
    s_active = true;
    Serial.println("[Plaud] Recording started — screen off.");
}

static void stop_recording()
{
    micCaptureStop();
    displaySetBacklight(true);
    s_active = false;
    uiStatusBarSetQueueCount(sdCardCountQueueFiles());
    Serial.println("[Plaud] Recording stopped — screen on.");
}

void plaudModeInit()
{
    bootButtonInit();
    micCaptureInit();
}

void plaudModeHandle()
{
    if (bootButtonConsumePress()) {
        if (s_active) stop_recording();
        else          start_recording();
    }

    if (s_active) {
        micCaptureHandle();       // LVGL is intentionally not pumped while recording
    } else {
        lv_timer_handler();
    }
}

bool plaudModeIsActive()
{
    return s_active;
}
