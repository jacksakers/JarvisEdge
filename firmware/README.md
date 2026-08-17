# Jarvis Edge Node — Firmware

PlatformIO/Arduino firmware for the Elecrow 3.5" ESP32-S3 HMI touchscreen.
Implements **Phases 1, 2, and 4** of [../docs/plan.txt](../docs/plan.txt):
hardware bring-up, the swipeable LVGL UI shell, the SD-backed offline audio
queue ("Plaud mode"), and WiFi/MQTT sync with the [Phase 3 backend](../backend).

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
main.cpp             setup()/loop() — display+UI init, SD mount, WiFi/MQTT/sync init, plaudModeHandle()+wifi/mqtt drive the loop
display.h/.cpp        LovyanGFX panel + GT911 touch bring-up, LVGL glue, backlight control
include/LovyanGFX_Driver.h   Panel/touch pin map (LGFX class)
include/lv_conf.h     LVGL feature configuration for this project
ui.h/.cpp             Builds the status bar + tile carousel on the active screen
ui_status_bar.h/.cpp  "Persistent Status Bar" (WiFi/LoRa/battery/queue icons)
ui_screen_feed.h/.cpp     "Jarvis Feed" — home tile, big AI-response label
ui_screen_focus.h/.cpp    "Daily Focus" — 3 tappable to-do items
ui_screen_actions.h/.cpp  "Action Grid" — 2x2 manual trigger buttons
ui_screen_settings.h/.cpp "Settings" — on-device WiFi/backend/MQTT config form
sd_card.h/.cpp        Mounts the SD card (SPI3/HSPI) and manages /queue
boot_button.h/.cpp     BOOT-button (GPIO0) interrupt — minimal ISR + debounced consume
mic_capture.h/.cpp     I2S PDM mic -> WAV-on-SD streaming capture (double-buffered)
plaud_mode.h/.cpp      Orchestrates BOOT press -> backlight off + mic capture -> WAV in /queue
network_config.h       Compile-time WiFi SSID/password, backend host/port, MQTT host/port/topics (fallback defaults only)
settings.h/.cpp        SD-backed (/settings/jarvis.txt) runtime-editable settings store, overrides network_config.h
wifi_manager.h/.cpp    Non-blocking WiFi connect + status bar icon updates; wifiManagerReconnect() for Settings save
mqtt_client.h/.cpp     PubSubClient subscribe to jarvis/ui/feed + jarvis/ui/focus, updates UI tiles; mqttClientReconnect() for Settings save
sync_manager.h/.cpp    Core-0 FreeRTOS task: uploads queued /queue/*.wav to the backend, deletes on success
edge_api.h/.cpp        Fire-and-forget POSTs to the backend (focus toggle, action triggers) from Core-0 FreeRTOS tasks so LVGL taps never block on HTTP
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
Focus item strikes it through locally). Phase 5 adds a fourth tile,
**Settings**, after Action Grid.

### On-device Settings (docs/plan.txt Phase 5)

`ui_screen_settings.cpp` builds a scrollable form with six text fields —
WiFi SSID, WiFi password (masked), backend host, backend port, MQTT host,
MQTT port — backed by an `lv_keyboard` overlay that pops up on focus
(numeric mode for the port fields, lowercase text otherwise) and hides on
`READY`/`CANCEL`. Values are seeded from `settings.h` getters (SD-stored
value if present, else the `network_config.h` compile-time default). A
"Save & Reconnect" button writes all six fields via `settingsSet*()` +
`settingsSave()` (persisted to `/settings/jarvis.txt` on the SD card), then
calls `wifiManagerReconnect()` and `mqttClientReconnect()` so the new WiFi/
backend/MQTT config takes effect immediately without a reboot.

Requires `LV_USE_KEYBOARD` and `LV_USE_TEXTAREA` enabled in `lv_conf.h`
(both were off by default and had to be turned on for this feature).

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

### WiFi + MQTT sync (docs/sdd.txt section 4.2)

`wifi_manager` connects to the SSID/password compiled into
`network_config.h` (override via `build_flags`) and updates the status bar
WiFi icon on connect/disconnect transitions. `mqtt_client` then subscribes to
two topics the backend publishes on:

```
jarvis/ui/feed  -> {"text": "..."}                                  -> uiFeedSetText()
jarvis/ui/focus -> {"tasks": [{"id": 1, "text": "..."}, ...]}       -> uiFocusSetItemSynced(0..2, id, text)
```

Each focus task carries the backend `FocusItem.id`. Tapping a Daily Focus
row on-device optimistically strikes it through locally and fires
`edgeApiToggleFocus(id)` (via `edge_api.cpp`, on its own short-lived Core-0
task) so `POST /focus/{id}/toggle` runs in the background without blocking
the UI thread — the Command Center and any other viewer then see the same
state over MQTT.

The Action Grid tile works the same way: tapping Time Track/Dismiss fires
`edgeApiTriggerAction(type, "")` immediately, while Note/Alert pop an
`lv_keyboard` overlay (built in `ui_screen_actions.cpp`) so you can type
before it POSTs to `/actions/{action_type}`.

Separately, `sync_manager` runs its own FreeRTOS task pinned to core 0 (like
the mic capture writer). Every 15s it checks WiFi is up, the SD card is
mounted, and Plaud mode isn't actively recording, then walks `/queue`,
uploads each `.wav` to the backend's `POST /upload/audio` (multipart body
built in PSRAM, capped at 8 MB per file), and deletes the file locally on a
200 response. This keeps the auto-sync loop off the LVGL/touch-handling core
and out of the way of a live recording.

### Not yet implemented (see ../docs/plan.txt)

None — Phases 1, 2, 4, and 5 (device-side) are all implemented. The other
half of Phase 5, the Vite Command Center web frontend, lives in
[../frontend](../frontend) and talks to the [backend](../backend) rather
than the firmware.
