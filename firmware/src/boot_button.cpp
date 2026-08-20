// Project  : House Phone
// File     : boot_button.cpp
// Purpose  : Hardware BOOT-button interrupt — toggles "Plaud mode" recording
// Depends  : boot_button.h

#include "boot_button.h"
#include <Arduino.h>

#define BOOT_BUTTON_PIN  0     /* Standard ESP32/-S3 BOOT/GPIO0, active-low */
#define DEBOUNCE_MS    250

static volatile bool s_isr_flag = false;
static unsigned long s_last_press_ms = 0;

static void IRAM_ATTR boot_isr()
{
    s_isr_flag = true;   // ISR body kept minimal — see docs/coding.txt 2.2
}

void bootButtonInit()
{
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), boot_isr, FALLING);
    Serial.println("[Button] BOOT button interrupt armed (GPIO0).");
}

bool bootButtonConsumePress()
{
    if (!s_isr_flag) return false;
    s_isr_flag = false;

    unsigned long now = millis();
    if (now - s_last_press_ms < DEBOUNCE_MS) return false;
    s_last_press_ms = now;
    return true;
}
