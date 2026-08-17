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
  frontend/    Vite + React admin frontend ("Command Center": Feed/Focus/Actions/Logs/JARVIS/Prompts/Settings)
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

**Phase 6 — Full CRUD, JARVIS 3.0 integration & Command Center rebuild: implemented.**
The Daily Focus and Action Grid tiles now actually do something end-to-end:

- **Backend** gained full CRUD for `FocusItem` (`/focus*`) and `ActionEvent`
  (`/actions/*`), plus an optional best-effort bridge to a full JARVIS 3.0
  instance (`app/jarvis_client.py`, `/jarvis/status`, `/jarvis/feed`) —
  notes can be forwarded to JARVIS's journal and heavy-tier tasks can be
  delegated to it. See [backend/README.md](backend/README.md).
- **Firmware** gained `edge_api.h`/`.cpp`, a small fire-and-forget HTTP
  client (Core-0 FreeRTOS tasks) so tapping a Daily Focus item or an Action
  Grid tile actually calls the backend instead of just updating local state.
  Daily Focus items now carry a backend id (via the `jarvis/ui/focus` MQTT
  payload) so a tap syncs back with `POST /focus/{id}/toggle`; Note/Alert
  tiles pop an on-device keyboard overlay before POSTing to
  `/actions/{action_type}`. See [firmware/README.md](firmware/README.md).
- **Frontend** was fully rebuilt as a proper Command Center: a
  glass/cyan design system shared with JARVIS 3.0, client-side routing
  (`react-router-dom`), and dedicated pages for the live Jarvis Feed +
  quick actions, Daily Focus CRUD, the Action Grid (trigger + history),
  Voice Logs, JARVIS 3.0 link status, Prompts, and Settings (now including
  the JARVIS enable/base-URL fields and a Test Connection button). See
  [frontend/README.md](frontend/README.md).

Note: raw audio is deleted from the device queue and the backend right
after transcription (privacy-by-design), so there's no stored audio to
visualize — the frontend's "waveform" is a decorative activity indicator
tied to log status, not a literal recording playback.
