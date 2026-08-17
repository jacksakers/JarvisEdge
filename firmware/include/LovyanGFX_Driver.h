// Project  : Jarvis Edge Node
// File     : LovyanGFX_Driver.h
// Purpose  : LovyanGFX hardware configuration for the Elecrow 3.5" ESP32-S3 HMI display
// Depends  : LovyanGFX library
//
// NOTE: Pin mapping and panel quirks below are carried over verbatim from the
// HelpDesk firmware, which targets the same Elecrow CrowPanel Advance 3.5"
// ESP32-S3 HMI board family described in ../docs/sdd.txt (ILI9488, 480x320,
// capacitive touch). Re-verify against the actual Jarvis Edge Node hardware
// once it is in hand — if it turns out to be a different Elecrow revision,
// only this file should need to change.

// ─── Pin map (Elecrow CrowPanel Advance 3.5" ESP32-S3 HMI, 480x320) ────────
//   SPI SCLK  : 42      Backlight : 38
//   SPI MOSI  : 39      I2C SDA   : 15  (touch)
//   SPI CS    : 40      I2C SCL   : 16  (touch)
//   SPI D/C   : 41      Touch INT : 47
//   Touch I2C address : 0x14 (GT911, alternate addr)
//
//   NOTE: ILI9488 over SPI defaults to 18-bit color (3 bytes/pixel).
//   Do NOT call gfx.setColorDepth(16) — the panel likely ignores COLMOD 0x55
//   and stays in 18-bit mode.  LovyanGFX handles the RGB565→RGB666 conversion
//   automatically inside pushImage when the source type is rgb565_t.

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Touch_GT911   _touch_instance;

public:
    LGFX(void)
    {
        // ── SPI bus ──────────────────────────────────────────────────────
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 40000000;   // 40 MHz — 80 MHz caused garbled LVGL output
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = false;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = 42;
            cfg.pin_mosi    = 39;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = 41;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // ── Display panel (ILI9488, 480x320, landscape) ───────────────────
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 40;
            cfg.pin_rst          = -1;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 320;
            cfg.memory_height    = 480;
            cfg.panel_width      = 320;
            cfg.panel_height     = 480;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 1;   // landscape, USB-right orientation
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;    // IPS panel needs color inversion
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = true;
            _panel_instance.config(cfg);
        }

        // ── Touch (GT911 via I2C) ────────────────────────────────────────
        {
            auto cfg = _touch_instance.config();
            cfg.x_min          = 0;
            cfg.x_max          = 319;     // GT911 hardware is portrait; x/y match physical sensor
            cfg.y_min          = 0;
            cfg.y_max          = 479;
            cfg.pin_int        = 47;
            cfg.bus_shared     = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port       = 0;
            cfg.i2c_addr       = 0x14;
            cfg.pin_sda        = 15;
            cfg.pin_scl        = 16;
            cfg.freq           = 400000;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};
