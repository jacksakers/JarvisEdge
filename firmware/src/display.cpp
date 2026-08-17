// Project  : Jarvis Edge Node
// File     : display.cpp
// Purpose  : Initialise LovyanGFX driver, touch, and LVGL (LVGL v9 API)
// Depends  : LovyanGFX_Driver.h, lvgl.h

#include <lvgl.h>
#include "display.h"
#include "settings.h"
#include "LovyanGFX_Driver.h"
#include <Arduino.h>
#include <Wire.h>

#define DISPLAY_BACKLIGHT_PIN  38

// ─── Hardware instance ──────────────────────────────────────────────────────
static LGFX gfx;

// ─── LVGL draw buffer ────────────────────────────────────────────────────────
// Raw uint16_t buffer for RGB565. LovyanGFX converts RGB565→RGB666 automatically
// during pushImage when the source type is lgfx::rgb565_t (ILI9488 is 18-bit on SPI).
#define DRAW_BUF_PX   (480 * 320 / 10)           // 15360 pixels
static uint16_t      draw_buf_arr[DRAW_BUF_PX];
static lv_display_t * disp = nullptr;

static uint32_t my_tick(void)
{
    return millis();
}

static void my_disp_flush(lv_display_t * d, const lv_area_t * area, uint8_t * px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx.pushImage(area->x1, area->y1, w, h, (lgfx::rgb565_t *)px_map);
    lv_display_flush_ready(d);
}

static uint32_t s_last_activity_ms = 0;
static bool s_backlight_on = true;
static bool s_screen_locked = false;

static void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    if(gfx.getTouch(&data->point.x, &data->point.y)) {
        s_last_activity_ms = millis();
        if (!s_backlight_on) {
            displaySetBacklight(true);
            if (settingsGetScreenLockEnabled()) {
                s_screen_locked = true;
                Serial.println("[Display] Woke up - Screen is locked.");
            }

            data->state = LV_INDEV_STATE_RELEASED;

            return;
        }

        if (s_screen_locked) {
            s_screen_locked = false;
            Serial.println("[Display] Screen unlocked.");
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }

        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void initLVGL()
{
    lv_init();
    lv_tick_set_cb(my_tick);

    disp = lv_display_create(480, 320);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf_arr, NULL,
                           sizeof(draw_buf_arr),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
}

void initDisplay()
{
    Serial.println("[Display] Starting initDisplay...");
    Serial.printf("[Display] Free heap: %u  Free PSRAM: %u\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());

    gfx.init();

    // !! DO NOT call gfx.setColorDepth(16) !!
    // ILI9488 over SPI defaults to 18-bit (3 bytes/pixel) and many panels
    // ignore the COLMOD 0x55 command. LovyanGFX auto-converts RGB565→RGB666
    // inside pushImage as long as the panel is left at its default depth.

    pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(DISPLAY_BACKLIGHT_PIN, HIGH);

    gfx.startWrite();
    gfx.fillScreen(TFT_BLACK);

    initLVGL();

    Serial.println("[Display] Ready (480x320, RGB565, GT911 touch).");
}

void displaySetBacklight(bool on)
{
    digitalWrite(DISPLAY_BACKLIGHT_PIN, on ? HIGH : LOW);
    s_backlight_on = on;
    if (on) {
        s_last_activity_ms = millis();
    }
}


void displayHandle(unsigned long now)
{
    int timeout_sec = settingsGetScreenOffTimeout();
    if (s_backlight_on && timeout_sec > 0) {
        if (now - s_last_activity_ms > (unsigned long)timeout_sec * 1000UL) {
            Serial.println("[Display] Screen idle timeout - Turning backlight off.");
            displaySetBacklight(false);
        }
    }
}
