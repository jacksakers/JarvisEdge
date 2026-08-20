# House Phone — Firmware

PlatformIO/Arduino firmware for the Elecrow 3.5" ESP32-S3 HMI touchscreen.
Implements the House Phone carousel described in
[../docs/sdd.txt](../docs/sdd.txt): Ambient Home (Tapo), Landline Feed
(BLE notifications), Timers & Alarms, Jarvis Voice Capture, and Settings —
plus the SD-backed offline audio queue ("Plaud mode") and WiFi/MQTT sync
with the [backend](../backend) carried over from the original Jarvis Edge
Node design.

## Hardware

- MCU: ESP32-S3 (dual-core, PSRAM, BLE + WiFi)
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
3.5" ESP32-S3 board family — re-verify against your actual board if it
differs. **The Timers & Alarms buzzer pin
(`TIMER_BUZZER_PIN` in [src/timers_alarms.cpp](src/timers_alarms.cpp)) has
not been confirmed against real hardware yet** — check it before flashing.

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
main.cpp             setup()/loop() — display+UI init, SD mount, WiFi/MQTT/sync/Tapo/BLE/Timers init, drives every module's Handle()
display.h/.cpp        LovyanGFX panel + GT911 touch bring-up, LVGL glue, backlight control
include/LovyanGFX_Driver.h   Panel/touch pin map (LGFX class)
include/lv_conf.h     LVGL feature configuration for this project
ui.h/.cpp             Builds the status bar + 5-tile carousel on the active screen
ui_status_bar.h/.cpp  "Persistent Status Bar" (WiFi/LoRa/battery/queue icons)
ui_screen_home.h/.cpp      "Ambient Home" — Tapo bulb zone grid (tap toggle / long-press brightness / All Off)
ui_screen_landline.h/.cpp  "Landline Feed" — BLE-relayed phone notification cards
ui_screen_timers.h/.cpp    "Timers & Alarms" — countdown presets + bedside alarm config
ui_screen_voice.h/.cpp     "Jarvis Voice Capture" — record button + big AI-response label (formerly "Jarvis Feed")
ui_screen_settings.h/.cpp  "Settings" — on-device WiFi/backend/MQTT/VAD/power config form
tapo_control.h/.cpp   Ambient Home's backend client — polls GET /tapo/zones, fire-and-forget POSTs for toggle/brightness/all_off
ble_notifications.h/.cpp  BLE GATT server — receives Tasker-relayed phone notifications into an in-RAM ring buffer
timers_alarms.h/.cpp  Non-blocking countdown timer + NTP-checked wall-clock alarm, drives the onboard buzzer
sd_card.h/.cpp        Mounts the SD card (SPI3/HSPI) and manages /queue
boot_button.h/.cpp     BOOT-button (GPIO0) interrupt — minimal ISR + debounced consume
mic_capture.h/.cpp     I2S PDM mic -> WAV-on-SD streaming capture (double-buffered)
plaud_mode.h/.cpp      Orchestrates BOOT press -> backlight off + mic capture -> WAV in /queue
network_config.h       Compile-time WiFi SSID/password, backend host/port, MQTT host/port/topic (fallback defaults only)
settings.h/.cpp        SD-backed (/settings/jarvis.txt) runtime-editable settings store (WiFi/backend/MQTT/VAD/power/alarm), overrides network_config.h
wifi_manager.h/.cpp    Non-blocking WiFi connect + status bar icon updates + NTP sync (needed for the wall-clock alarm); wifiManagerReconnect() for Settings save
mqtt_client.h/.cpp     PubSubClient subscribe to jarvis/ui/feed, updates the Voice Capture tile; mqttClientReconnect() for Settings save
sync_manager.h/.cpp    Core-0 FreeRTOS task: uploads queued /queue/*.wav to the backend, deletes on success
```

### UI shell (docs/sdd.txt section 3)

The screen is a persistent 28px status bar over a horizontal `lv_tileview`
carousel — a smartwatch-style swipe between five tiles, with **Ambient
Home** as the home/default tile:

```
[ Ambient Home (home) ] <—swipe—> [ Landline Feed ] <—swipe—> [ Timers & Alarms ] <—swipe—> [ Jarvis Voice Capture ] <—swipe—> [ Settings ]
```

### Ambient Home (docs/sdd.txt 4.3)

`tapo_control.cpp` polls the backend's `GET /tapo/zones` every 4s (a plain
blocking `HTTPClient` call in `loop()`, same tradeoff `device_heartbeat.cpp`
already makes) and repaints `ui_screen_home.cpp`'s grid from the response.
Tapping a zone tile fires `tapoControlToggle()` on a short-lived Core-0
FreeRTOS task (mirrors the old `edge_api.cpp` pattern) so a slow/unreachable
backend never blocks LVGL; long-pressing opens a full-tile brightness
slider that calls `tapoControlSetBrightness()` on release. The backend does
the actual Tapo KLAP handshake (see `../backend/app/tapo.py`) — the ESP32
never talks to a bulb directly.

### Landline Feed (docs/sdd.txt 4.4)

`ble_notifications.cpp` starts a BLE GATT server (`BLEDevice.h`, bundled
with arduino-esp32) advertising as **"House Phone"**, with one writable
characteristic. Configure a Tasker profile on your phone with a BLE/GATT
plugin to write a JSON payload to that characteristic whenever a
notification posts:

```json
{"app": "Messages", "title": "Jane Doe", "text": "On my way!", "ts": 1699999999}
```

See the schema notes and characteristic UUIDs in
[include/ble_notifications.h](include/ble_notifications.h). Notifications
are held in an 8-entry in-RAM ring buffer only — never persisted to SD or
the backend, matching a real phone's ephemeral notification shade. BLE
write callbacks run on the Bluedroid/NimBLE host task, not the LVGL thread
— the callback only sets a flag + copies data into the ring buffer, and
`uiLandlineScreenHandle()` (called once per `loop()`) does the actual LVGL
refresh, same pattern as the BOOT button ISR (docs/coding.txt).

### Timers & Alarms (docs/sdd.txt 4.5)

`timers_alarms.cpp` runs a non-blocking countdown (three cooking presets on
`ui_screen_timers.cpp`) and a wall-clock alarm compared against NTP-synced
local time (`wifi_manager.cpp` already calls `configTzTime()` on WiFi
connect — the alarm won't be accurate until that's happened at least once
since boot). Both share the onboard buzzer (`tone()`/`noTone()`); while
ringing, a full-tile red overlay blocks touch until "Tap to silence". The
alarm hour/minute/enabled persist to SD via `settings.h`, same file as the
rest of the on-device config.

### On-device Settings (docs/plan.txt Phase 5)

`ui_screen_settings.cpp` builds a scrollable form with six text fields —
WiFi SSID, WiFi password (masked), backend host, backend port, MQTT host,
MQTT port — plus the Ambient VAD/power-saving checkboxes, backed by an
`lv_keyboard` overlay that pops up on focus (numeric mode for the port
fields, lowercase text otherwise) and hides on `READY`/`CANCEL`. A "Save &
Reconnect" button persists everything to `/settings/jarvis.txt` on the SD
card, then calls `wifiManagerReconnect()` and `mqttClientReconnect()` so the
new config takes effect immediately without a reboot. Tapo account
credentials and zone CRUD are Command-Center-only (too much typing for an
on-screen keyboard) — see [../backend/README.md](../backend/README.md).

Requires `LV_USE_KEYBOARD`, `LV_USE_TEXTAREA`, `LV_USE_SLIDER`,
`LV_USE_SWITCH`, and `LV_USE_ROLLER` enabled in `lv_conf.h` (the last one
was off by default and had to be turned on for the Timers & Alarms tile).

### Offline audio capture — "Plaud mode" (docs/sdd.txt section 4.1)

Pressing the BOOT button toggles a mutually-exclusive main-loop mode:

```
BOOT pressed (idle)     -> backlight off, open /queue/log_<ts>.wav, start I2S capture
BOOT pressed (recording) -> stop I2S capture, patch WAV header, close file, backlight on
```

While recording, `plaud_mode.cpp` calls `micCaptureHandle()` instead of
`lv_timer_handler()` each loop iteration — LVGL is intentionally not pumped
while the screen is off. The BOOT ISR only sets a flag (per docs/coding.txt
2.2); all actual work happens in `plaudModeHandle()` on the main loop.

`mic_capture.cpp` reads the onboard PDM mic via the legacy `driver/i2s.h`
API into a PSRAM buffer (the mic and the SD card share hardware and can't
run at the same time — the whole recording is buffered, then written to SD
once, after `i2s_mic_uninstall()`).

### WiFi + MQTT sync (docs/sdd.txt 4.2)

`wifi_manager` connects to the SSID/password compiled into
`network_config.h` (override via `build_flags`), updates the status bar
WiFi icon on connect/disconnect transitions, and syncs NTP time (needed by
the Timers & Alarms wall-clock alarm). `mqtt_client` then subscribes to the
one topic the backend publishes on:

```
jarvis/ui/feed -> {"text": "..."} -> uiVoiceSetText()
```

Separately, `sync_manager` runs its own FreeRTOS task pinned to core 0 (like
the mic capture writer, `tapo_control`'s polling stays on the main loop
instead). Every 15s it checks WiFi is up, the SD card is mounted, and Plaud
mode isn't actively recording, then walks `/queue`, uploads each `.wav` to
the backend's `POST /upload/audio` (multipart body built in PSRAM, capped at
8 MB per file), and deletes the file locally on a 200 response.

### Not yet implemented

- Direct smart-light control from a Landline Feed notification.
- BLE-side read acknowledgement back to Tasker.

See [../docs/plan.txt](../docs/plan.txt) for the full phase history.

