// Project  : House Phone
// File     : timers_alarms.cpp
// Purpose  : Cooking countdown timer + bedside wall-clock alarm
// Depends  : timers_alarms.h, settings.h, <time.h>

#include "timers_alarms.h"
#include "settings.h"
#include <Arduino.h>
#include <time.h>

// NOT verified against real House Phone hardware — pick a pin known to be
// free of the display/touch/SD/mic buses (docs comment: see mic_capture.h's
// pin-mapping caveat for the same board family) and re-check before flashing.
#define TIMER_BUZZER_PIN   21
#define BEEP_FREQ_HZ       2000
#define BEEP_ON_MS         300
#define BEEP_OFF_MS        400

static long          s_countdown_target_s = -1;   // epoch-ish seconds via millis(), -1 = idle
static unsigned long s_countdown_start_ms  = 0;
static int           s_countdown_total_s   = 0;

static int  s_alarm_hour    = -1;
static int  s_alarm_minute  = -1;
static bool s_alarm_enabled = false;
static int  s_alarm_last_fired_minute = -1;   // guards against re-firing all 60s of the match minute

static bool          s_ringing      = false;
static bool          s_buzzer_on    = false;
static unsigned long s_last_toggle_ms = 0;

void timersAlarmsInit()
{
    pinMode(TIMER_BUZZER_PIN, OUTPUT);
    digitalWrite(TIMER_BUZZER_PIN, LOW);

    bool enabled;
    int hour, minute;
    settingsGetAlarm(&hour, &minute, &enabled);
    s_alarm_hour = hour;
    s_alarm_minute = minute;
    s_alarm_enabled = enabled;

    Serial.printf("[Timers] Alarm restored: %02d:%02d (%s)\n",
                  s_alarm_hour < 0 ? 0 : s_alarm_hour, s_alarm_minute < 0 ? 0 : s_alarm_minute,
                  s_alarm_enabled ? "enabled" : "disabled");
}

static void start_ringing()
{
    s_ringing = true;
    s_last_toggle_ms = 0;   // force an immediate beep on the next handle()
}

void timersAlarmsSilence()
{
    s_ringing = false;
    s_buzzer_on = false;
    noTone(TIMER_BUZZER_PIN);
    s_countdown_target_s = -1;
    s_alarm_last_fired_minute = (s_alarm_hour * 60 + s_alarm_minute);   // don't re-fire this same minute
}

void timersAlarmsStartCountdown(int seconds)
{
    if (seconds <= 0) return;
    s_countdown_start_ms = millis();
    s_countdown_total_s = seconds;
    s_countdown_target_s = seconds;
}

void timersAlarmsCancelCountdown()
{
    s_countdown_target_s = -1;
    if (s_ringing) timersAlarmsSilence();
}

int timersAlarmsGetRemainingSeconds()
{
    if (s_countdown_target_s < 0) return -1;
    long elapsed = (millis() - s_countdown_start_ms) / 1000UL;
    long remaining = s_countdown_total_s - elapsed;
    return remaining > 0 ? (int)remaining : 0;
}

void timersAlarmsSetAlarm(int hour, int minute, bool enabled)
{
    s_alarm_hour = hour;
    s_alarm_minute = minute;
    s_alarm_enabled = enabled;
    settingsSetAlarm(hour, minute, enabled);
    settingsSave();
}

void timersAlarmsGetAlarm(int * hour, int * minute, bool * enabled)
{
    if (hour) *hour = s_alarm_hour;
    if (minute) *minute = s_alarm_minute;
    if (enabled) *enabled = s_alarm_enabled;
}

bool timersAlarmsIsRinging()
{
    return s_ringing;
}

void timersAlarmsHandle(unsigned long now)
{
    // Countdown: fire once when it hits zero.
    if (s_countdown_target_s >= 0 && !s_ringing) {
        if (timersAlarmsGetRemainingSeconds() <= 0) {
            Serial.println("[Timers] Countdown finished — ringing.");
            start_ringing();
        }
    }

    // Wall-clock alarm — only meaningful once NTP has actually set the time
    // (wifi_manager.cpp calls configTzTime() on connect).
    if (s_alarm_enabled && !s_ringing) {
        time_t t = time(nullptr);
        if (t > 100000) {   // sanity check — before NTP sync, time() returns ~0
            struct tm local;
            localtime_r(&t, &local);
            int minute_of_day = local.tm_hour * 60 + local.tm_min;
            int target_minute_of_day = s_alarm_hour * 60 + s_alarm_minute;
            if (minute_of_day == target_minute_of_day && s_alarm_last_fired_minute != minute_of_day) {
                Serial.println("[Timers] Alarm time reached — ringing.");
                s_alarm_last_fired_minute = minute_of_day;
                start_ringing();
            }
        }
    }

    // Non-blocking beep pattern while ringing (silenced by timersAlarmsSilence()).
    if (s_ringing) {
        if (now - s_last_toggle_ms >= (unsigned long)(s_buzzer_on ? BEEP_ON_MS : BEEP_OFF_MS)) {
            s_last_toggle_ms = now;
            s_buzzer_on = !s_buzzer_on;
            if (s_buzzer_on) tone(TIMER_BUZZER_PIN, BEEP_FREQ_HZ);
            else noTone(TIMER_BUZZER_PIN);
        }
    }
}
