# Jarvis Edge Node — Firmware

PlatformIO/Arduino firmware for the Elecrow 3.5" ESP32-S3 HMI touchscreen.
Implements **Phase 1** of [../docs/plan.txt](../docs/plan.txt): hardware
bring-up and the swipeable LVGL UI shell. No networking, SD, or audio yet —
those land in later phases.

## Hardware

- MCU: ESP32-S3 (dual-core, PSRAM)
- Display: ILI9488, 480x320, SPI, landscape
- Touch: GT911 capacitive touch controller over I2C
- Graphics driver: [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- UI toolkit: [LVGL](https://lvgl.io/) v9.2

Pin mapping lives in [include/LovyanGFX_Driver.h](include/LovyanGFX_Driver.h)
and was carried over from the `HelpDesk` firmware, which targets the same
Elecrow CrowPanel Advance 3.5" ESP32-S3 board family. Re-verify against the
real Jarvis Edge Node board once available — that file is the only place
pin numbers should need to change.

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
main.cpp            setup()/loop() — calls initDisplay() then ui_init()
display.h/.cpp       LovyanGFX panel + GT911 touch bring-up, LVGL glue
include/LovyanGFX_Driver.h   Panel/touch pin map (LGFX class)
include/lv_conf.h    LVGL feature configuration for this project
ui.h/.cpp            Builds the status bar + tile carousel on the active screen
ui_status_bar.h/.cpp "Persistent Status Bar" (WiFi/LoRa/battery/queue icons)
ui_screen_feed.h/.cpp    "Jarvis Feed" — home tile, big AI-response label
ui_screen_focus.h/.cpp   "Daily Focus" — 3 tappable to-do items
ui_screen_actions.h/.cpp "Action Grid" — 2x2 manual trigger buttons
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

### Not yet implemented (see ../docs/plan.txt)

- Phase 2: SD card offline audio queue, I2S mic capture, BOOT-button
  "Plaud mode" (backlight off + direct-to-SD recording)
- Phase 3: FastAPI backend, transcription, dual-tier LLM routing
- Phase 4: WiFi + MQTT sync between the device and the backend
- Phase 5: Vite admin frontend
