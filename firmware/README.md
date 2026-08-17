# Jarvis Edge Node — Firmware

PlatformIO/Arduino firmware for the Elecrow 3.5" ESP32-S3 HMI touchscreen.
Implements **Phases 1-2** of [../docs/plan.txt](../docs/plan.txt): hardware
bring-up, the swipeable LVGL UI shell, and the SD-backed offline audio queue
("Plaud mode"). No WiFi/MQTT/backend yet — those land in Phase 3+.

## Hardware

- MCU: ESP32-S3 (dual-core, PSRAM)
- Display: ILI9488, 480x320, SPI, landscape
- Touch: GT911 capacitive touch controller over I2C
- SD card: dedicated SPI3/HSPI bus (separate from the display's SPI2)
- Mic: onboard PDM microphone over I2S0
- Graphics driver: [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- UI toolkit: [LVGL](https://lvgl.io/) v9.2

Pin mapping lives in [include/LovyanGFX_Driver.h](include/LovyanGFX_Driver.h)
(display/touch), [src/sd_card.cpp](src/sd_card.cpp) (SD), and
[include/mic_capture.h](include/mic_capture.h) (mic), all carried over from
the `HelpDesk` firmware, which targets the same Elecrow CrowPanel Advance
3.5" ESP32-S3 board family. Re-verify against the real Jarvis Edge Node
board once available — those are the only places pin numbers should need
to change.

## Build & flash

Requires [PlatformIO](https://platformio.org/) (CLI or the VS Code
extension).

```bash
cd firmware
pio run              # build
pio run -t upload    # flash over USB
pio device monitor    # serial console (460800 baud)
```

`platformio.ini` pins the board to `esp32-s3-devkitc-1` with DIO flash mode
and a 16 MB partition table — override these if the real hardware differs.

## Architecture

```
main.cpp             setup()/loop() — display+UI init, SD mount, plaudModeHandle() drives the loop
display.h/.cpp        LovyanGFX panel + GT911 touch bring-up, LVGL glue, backlight control
include/LovyanGFX_Driver.h   Panel/touch pin map (LGFX class)
include/lv_conf.h     LVGL feature configuration for this project
ui.h/.cpp             Builds the status bar + tile carousel on the active screen
ui_status_bar.h/.cpp  "Persistent Status Bar" (WiFi/LoRa/battery/queue icons)
ui_screen_feed.h/.cpp     "Jarvis Feed" — home tile, big AI-response label
ui_screen_focus.h/.cpp    "Daily Focus" — 3 tappable to-do items
ui_screen_actions.h/.cpp  "Action Grid" — 2x2 manual trigger buttons
sd_card.h/.cpp        Mounts the SD card (SPI3/HSPI) and manages /queue
boot_button.h/.cpp     BOOT-button (GPIO0) interrupt — minimal ISR + debounced consume
mic_capture.h/.cpp     I2S PDM mic -> WAV-on-SD streaming capture (double-buffered)
plaud_mode.h/.cpp      Orchestrates BOOT press -> backlight off + mic capture -> WAV in /queue
```

### UI shell (docs/sdd.txt section 3)

The screen is a persistent 28px status bar over a horizontal `lv_tileview`
carousel — a smartwatch-style swipe between three tiles, with **Jarvis
Feed** as the home/default tile:

```
[ Daily Focus ] <—swipe—> [ Jarvis Feed (home) ] <—swipe—> [ Action Grid ]
```

Each screen module exposes setter functions (`uiFeedSetText`,
`uiFocusSetItem`, `uiStatusBarSetWifiConnected`, etc.) that later phases will
call once real WiFi/MQTT/queue state exists — Phase 1 only wires the
button/tap interactions that work entirely on-device (e.g. tapping a Daily
Focus item strikes it through locally).

### Offline audio capture — "Plaud mode" (docs/sdd.txt section 4.1)

Pressing the BOOT button toggles a mutually-exclusive main-loop mode:

```
BOOT pressed (idle)     -> backlight off, open /queue/log_<ts>.wav, start I2S capture
BOOT pressed (recording) -> stop I2S capture, patch WAV header, close file, backlight on
```

While recording, `plaud_mode.cpp` calls `micCaptureHandle()` instead of
`lv_timer_handler()` each loop iteration — LVGL is intentionally not pumped
while the screen is off, matching docs/plan.txt Phase 2 ("pause
lv_task_handler()"). The BOOT ISR only sets a flag (per docs/coding.txt 2.2);
all actual work happens in `plaudModeHandle()` on the main loop.

`mic_capture.cpp` reads the onboard PDM mic via the legacy `driver/i2s.h`
API into one of two 4 KB chunk buffers. When a buffer fills, it's handed off
to a FreeRTOS writer task pinned to core 0 while the main loop keeps filling
the other buffer — double buffering so a slow SD write never drops or
glitches an I2S frame. Recording length is unbounded (no fixed duration);
the WAV header is written as a zeroed placeholder up front and patched with
the final size when the recording stops.

### Not yet implemented (see ../docs/plan.txt)

- Phase 3: FastAPI backend, transcription, dual-tier LLM routing
- Phase 4: WiFi + MQTT sync between the device and the backend (uploading
  and deleting the queued `.wav` files currently just accumulate on the SD
  card)
- Phase 5: Vite admin frontend
