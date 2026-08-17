// Project  : Jarvis Edge Node
// File     : boot_button.h
// Purpose  : Hardware BOOT-button interrupt — public interface
// Depends  : <Arduino.h>
//
// Per docs/coding.txt 2.2: the ISR only sets a flag; all real work (starting
// or stopping a recording) happens in the main loop via bootButtonConsumePress().

#pragma once

// Arms the BOOT button (GPIO0) interrupt. Call once from setup().
void bootButtonInit();

// Returns true once per debounced press, and clears the pending flag.
// Call every loop() iteration.
bool bootButtonConsumePress();
