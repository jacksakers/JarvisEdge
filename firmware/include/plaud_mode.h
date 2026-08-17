// Project  : Jarvis Edge Node
// File     : plaud_mode.h
// Purpose  : "Plaud mode" state machine — BOOT button toggles screen-off audio capture
// Depends  : boot_button.h, mic_capture.h, display.h
//
// See docs/sdd.txt section 4.1 (Audio Capture & Offline Queuing).

#pragma once

// Arms the BOOT button and mic capture module. Call once from setup(),
// after initDisplay() and sdCardInit().
void plaudModeInit();

// Drives the whole state machine. Call every loop() iteration *instead of*
// lv_timer_handler() directly — this function calls it for you whenever
// Plaud mode is inactive, and drives mic capture instead while active.
void plaudModeHandle();

bool plaudModeIsActive();

// Toggles a manual, screen-on recording (e.g. from the status bar's record
// button) — unlike BOOT, this never touches the backlight. A no-op (returns
// false) while a BOOT-triggered recording already owns the mic. Returns true
// if a recording is now active.
bool plaudModeToggleManualRecording();
