// Project  : House Phone
// File     : display.h
// Purpose  : Display, touch, and LVGL initialisation — public interface
// Depends  : LovyanGFX_Driver.h, lvgl.h

#pragma once

// Initialise the LovyanGFX driver (panel + touch) and LVGL.
// Must be called before ui_init() in setup().
void initDisplay();

// Turns the backlight on/off. Used by Plaud mode to blank the screen

// during audio capture (see docs/sdd.txt section 4.1).

void displaySetBacklight(bool on);


// Handle idle timeouts and auto-dimming / pocket power-saving

void displayHandle(unsigned long now);

// Clears the screen-lock state — called when the lock overlay's swipe-up
// gesture is recognized (see ui.cpp).
void displayForceUnlock();
