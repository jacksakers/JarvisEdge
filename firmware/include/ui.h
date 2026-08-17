// Project  : Jarvis Edge Node
// File     : ui.h
// Purpose  : Shared layout constants and public entry point for the LVGL UI
// Depends  : lvgl.h

#pragma once
#include <lvgl.h>

// ── Screen geometry ──────────────────────────────────────────────────────
#define UI_SCREEN_W     480
#define UI_SCREEN_H     320
#define UI_STATUSBAR_H   28
#define UI_CAROUSEL_Y   UI_STATUSBAR_H
#define UI_CAROUSEL_H   (UI_SCREEN_H - UI_STATUSBAR_H)

// ── Palette (high-contrast dark theme — see docs/sdd.txt section 3) ─────
#define UI_CLR_BG        0x101018
#define UI_CLR_SURFACE   0x1A1A24
#define UI_CLR_ACCENT    0x2196F3
#define UI_CLR_TEXT      0xF5F5F5
#define UI_CLR_MUTED     0x8A8A99

// Tile indices within the carousel (Jarvis Feed is the default/home tile).
enum ui_tile_id_t {
    UI_TILE_FOCUS    = 0,
    UI_TILE_FEED     = 1,
    UI_TILE_ACTIONS  = 2,
    UI_TILE_SETTINGS = 3,
    UI_TILE_LOGS     = 4,

};

// Builds the persistent status bar + swipeable tile carousel on the active
// screen. Call once from setup(), after initDisplay().
void ui_init();
