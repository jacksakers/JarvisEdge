// Project  : House Phone
// File     : timers_alarms.h
// Purpose  : Cooking countdown timer + bedside wall-clock alarm — public
//            interface (docs/new_idea.txt section 4, Card 3)
// Depends  : settings.h
//
// The buzzer pin (TIMER_BUZZER_PIN in timers_alarms.cpp) has NOT been
// confirmed against a real House Phone board yet — verify/adjust before
// flashing hardware (same caveat as sd_card.cpp's pin map).

#pragma once

// Arms the buzzer output and restores the persisted alarm (if any) from
// settings.h. Call once from setup(), after settingsInit().
void timersAlarmsInit();

// Call every loop() iteration — ticks the countdown, checks the wall-clock
// alarm against the current time, and drives the ringing buzzer pattern.
void timersAlarmsHandle(unsigned long now);

// ── Cooking / countdown timer ─────────────────────────────────────────────
void timersAlarmsStartCountdown(int seconds);
void timersAlarmsCancelCountdown();
// Seconds left, or -1 if no countdown is running.
int  timersAlarmsGetRemainingSeconds();

// ── Bedside wall-clock alarm (requires WiFi/NTP for accurate time) ────────
void timersAlarmsSetAlarm(int hour, int minute, bool enabled);
void timersAlarmsGetAlarm(int * hour, int * minute, bool * enabled);

// True while the countdown or the alarm is actively ringing.
bool timersAlarmsIsRinging();
// Silences the buzzer and clears the ringing state (tap-to-dismiss).
void timersAlarmsSilence();
