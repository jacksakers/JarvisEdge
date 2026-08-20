// Project  : House Phone
// File     : plaud_mode.cpp
// Purpose  : "Plaud mode" state machine — BOOT button toggles screen-off audio capture
// Depends  : plaud_mode.h

#include "plaud_mode.h"
#include "boot_button.h"
#include "mic_capture.h"
#include "sd_card.h"
#include "display.h"
#include "ui_status_bar.h"
#include "settings.h"
#include <lvgl.h>

#include <Arduino.h>

static bool s_screen_off_active = false;   // BOOT-triggered — backlight killed
static bool s_manual_active     = false;   // on-screen button — screen stays on

#define VAD_BOOT_GRACE_MS      5000UL   /* let the PDM clock/DC estimate settle before trusting reads */
#define VAD_CHECK_INTERVAL_MS  1500UL
#define VAD_REQUIRED_HITS      2        /* consecutive hits needed before actually starting a recording */

static void start_recording()
{
    displaySetBacklight(false);   // kill the backlight before touching SD/I2S

    if (!micCaptureStart()) {
        displaySetBacklight(true);
        return;
    }
    s_screen_off_active = true;
    Serial.println("[Plaud] Recording started — screen off.");
}

static void stop_recording()
{
    micCaptureStop();
    displaySetBacklight(true);
    s_screen_off_active = false;
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
        if (s_screen_off_active)   stop_recording();
        else if (!s_manual_active) start_recording();   // BOOT can't steal the mic mid manual-recording
    }

    if (s_screen_off_active) {
        micCaptureHandle();       // screen off — LVGL intentionally not pumped
        return;
    }

    if (s_manual_active) {
        micCaptureHandle();   // keep filling the PSRAM buffer
    } else if (settingsGetAmbientVadEnabled()) {
        static unsigned long s_last_vad_check = 0;
        static uint8_t       s_vad_hits = 0;
        unsigned long now = millis();

        // The PDM clock/DC-bias estimate is unsettled for a moment right
        // after boot (and after any prior i2s install/uninstall cycle) —
        // reading amplitude too early sees a transient spike and falsely
        // "hears" speech, which is why enabling VAD used to blank the
        // screen immediately on every reboot. Give it a grace period.
        if (now >= VAD_BOOT_GRACE_MS && now - s_last_vad_check > VAD_CHECK_INTERVAL_MS) {
            s_last_vad_check = now;
            if (micCaptureDetectVAD()) {
                s_vad_hits++;
                // Require a couple of consecutive positive reads before
                // committing to a recording — filters out one-off spikes.
                if (s_vad_hits >= VAD_REQUIRED_HITS) {
                    s_vad_hits = 0;
                    Serial.println("[VAD] Voice confirmed — starting ambient screen-off recording.");
                    start_recording();
                }
            } else {
                s_vad_hits = 0;
            }
        }
    }
    lv_timer_handler();                        // screen stays on either way
}

bool plaudModeIsActive()
{
    return micCaptureIsActive();   // the fact SD/mic can't overlap doesn't care who triggered capture
}

bool plaudModeToggleManualRecording()
{
    if (s_screen_off_active) return false;   // BOOT-triggered recording already owns the mic

    if (s_manual_active) {
        micCaptureStop();
        s_manual_active = false;
        uiStatusBarSetQueueCount(sdCardCountQueueFiles());
        Serial.println("[Plaud] Manual recording stopped.");
        return false;
    }

    if (!micCaptureStart()) return false;
    s_manual_active = true;
    Serial.println("[Plaud] Manual recording started — screen stays on.");
    return true;
}
