# Jarvis Edge Node

A purpose-built, dedicated mobile hardware interface for the local home AI
framework ("Jarvis" — see the [JARVIS](../JARVIS) repo for the backend/
frontend). The Edge Node is a frictionless capture point for daily logs,
thoughts, and tasks, running on an Elecrow ESP32-S3 3.5" touchscreen with
LoRa, SD storage, and an onboard microphone.

## Documentation

- [docs/sdd.txt](docs/sdd.txt) — System Design Document: architecture, UI/UX,
  and core workflows across the three tiers (edge firmware, home server,
  admin frontend).
- [docs/plan.txt](docs/plan.txt) — Phased implementation roadmap.
- [docs/coding.txt](docs/coding.txt) — Coding standards for the C++/Python/JS
  stacks used across the project.

## Repository layout

```
JarvisEdge/
  docs/        System design, coding standards, and phased plan
  firmware/    ESP32-S3 PlatformIO project (LovyanGFX + LVGL UI shell)
  backend/     FastAPI home-server backend (audio ingestion, ASR, dual-tier LLM routing)
  frontend/    Vite + React admin frontend ("Command Center": Settings/Prompts/Data)
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
device.

**"address already in use" on port 8000?** That's a different, unrelated
project's dev server (not JarvisEdge) already bound to that port on this
machine. The backend now defaults to port **8010** instead (`config.yaml`
`server.port`) specifically to avoid that clash — pull the latest changes
and the conflict goes away. If your ESP32 was already flashed with the old
default backend port, open the **Settings** tile on the device and update
the backend port there (no reflash needed).

## Status

**Phase 1 — Hardware Baseline & UI Shell: implemented.**
**Phase 2 — Audio & Offline Mode ("Plaud" feature): implemented.**
**Phase 4 — Synchronization & MQTT: implemented.**
See [firmware/README.md](firmware/README.md) for build instructions and
architecture notes.

**Phase 3 — Backend API & AI Routing: implemented.**
See [backend/README.md](backend/README.md) for setup, configuration, and
the manual test procedure.

**Phase 5 — On-device Settings + Command Center: implemented.** The
firmware has a fourth carousel tile for managing WiFi/backend/MQTT config
from the device itself (see firmware/README.md); the backend exposes
`/settings`, `/models`, and `/prompts` (see backend/README.md); and
[frontend/](frontend) is a Vite + React app with Settings/Prompts/Data
pages consuming those endpoints. Run it with `cd frontend && npm install &&
npm run dev`.
