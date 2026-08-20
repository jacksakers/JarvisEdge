# House Phone

A purpose-built, dedicated mobile hardware interface that replaces your
smartphone once you're home. Built on an Elecrow ESP32-S3 3.5" touchscreen
with SD storage and an onboard microphone, it relays phone notifications
over BLE, controls Tapo smart lighting directly over the LAN, runs cooking
timers and a bedside alarm, and captures local AI voice notes through the
local home AI framework ("Jarvis" — see the [JARVIS](../JARVIS) repo for the
optional full agent backend/frontend). This project started as "Jarvis Edge
Node" — see [docs/new_idea.txt](docs/new_idea.txt) for the pivot proposal.

## Documentation

- [docs/sdd.txt](docs/sdd.txt) — System Design Document: architecture, UI/UX,
  and core workflows across the four tiers (edge firmware, home server,
  mobile bridge, admin frontend).
- [docs/plan.txt](docs/plan.txt) — Phased implementation roadmap.
- [docs/coding.txt](docs/coding.txt) — Coding standards for the C++/Python/JS
  stacks used across the project.
- [docs/new_idea.txt](docs/new_idea.txt) — the original House Phone pivot
  proposal (implemented — kept as the project's vision doc).

## Repository layout

```
JarvisEdge/
  docs/        System design, coding standards, and phased plan
  firmware/    ESP32-S3 PlatformIO project (LovyanGFX + LVGL UI shell)
  backend/     FastAPI home-server backend (audio ingestion, ASR, dual-tier LLM routing, Tapo control)
  frontend/    Vite + React admin frontend ("Command Center": Voice Capture/Ambient Home/Todo List/Voice Logs/JARVIS/Prompts/Settings)
```

## Quickstart

One-time setup (creates the backend venv + installs deps, installs frontend
npm packages):

```bash
./scripts/setup.sh
```

Then, each time you want to run it:

```bash
./scripts/start.sh
```

This starts the backend on **http://localhost:8010** and the frontend dev
server on **http://localhost:5180** — open the frontend URL in a browser,
that's the Command Center UI. Ctrl+C stops both.

Prerequisites the scripts don't install for you: a running
[Ollama](https://ollama.com) instance with the `fast_model`/`heavy_model`
from `backend/config.yaml` pulled, and (optionally) a local MQTT broker such
as Mosquitto — see [backend/README.md](backend/README.md) for details. The
backend still works without MQTT; it just won't push live updates to the
device. Ambient Home (Tapo bulb control) needs a Tapo account email/password
set in the Command Center's Settings page, and Landline Feed (BLE
notifications) needs a Tasker profile on your phone — see
[firmware/README.md](firmware/README.md).

**"address already in use" on port 8000?** That's a different, unrelated
project's dev server (not this one) already bound to that port on this
machine. The backend defaults to port **8010** instead (`config.yaml`
`server.port`) specifically to avoid that clash. If your ESP32 was already
flashed with the old default backend port, open the **Settings** tile on the
device and update the backend port there (no reflash needed).

## Status

**Jarvis Edge Node foundation (Phases 1-6): implemented.** Hardware
baseline + LVGL UI shell, offline audio queue + "Plaud mode", the FastAPI
backend's dual-tier LLM routing, WiFi/MQTT sync, the Command Center
frontend, and optional JARVIS 3.0 integration. See
[docs/plan.txt](docs/plan.txt) for the phase-by-phase history.

**House Phone pivot (Phases 7-10): implemented.** Three new carousel tiles
replace the old Daily Focus/Action Grid tiles:

- **Ambient Home** — a grid of Tapo bulb zones. Tap to toggle, long-press
  for brightness, "All Off" to clear the room. Control is routed through
  the backend's `app/tapo.py` (`python-kasa`, local KLAP protocol) rather
  than reimplementing Tapo's auth/crypto handshake in firmware — configure
  zones and your Tapo account login from the Command Center's Ambient Home
  and Settings pages. See [backend/README.md](backend/README.md).
- **Landline Feed** — a BLE GATT server (`firmware/src/ble_notifications.cpp`,
  advertises as "House Phone") that a Tasker profile on your phone can write
  Android notifications to; shown as dismissible cards. See
  [firmware/README.md](firmware/README.md) for the JSON schema and Tasker
  setup notes.
- **Timers & Alarms** — cooking countdown presets and a bedside wall-clock
  alarm, both driving the onboard buzzer, fully on-device
  (`firmware/src/timers_alarms.cpp`). The buzzer pin has not been confirmed
  against real hardware yet — see the file header before flashing.

The Jarvis Voice Capture tile (formerly "Jarvis Feed") and the on-device
Settings tile are unchanged. The old Daily Focus/Action Grid device tiles
are gone; the to-do list they fed still exists as a Command-Center-only
**Todo List** page (`frontend/src/pages/TodoPage.jsx`), still populated by
the heavy LLM tier's task extraction from voice notes.

Note: raw audio is deleted from the device queue right after transcription
(privacy-by-design), so there's no stored audio to visualize — the
frontend's "waveform" on the Voice Capture page is a decorative activity
indicator tied to log status, not a literal recording playback. BLE
notifications are similarly ephemeral — never persisted to SD or the
backend, matching a real phone's notification shade.

