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
```

A Vite admin frontend will be added per [docs/plan.txt](docs/plan.txt)
Phase 5; it will live in a sibling directory here once started.

## Status

**Phase 1 — Hardware Baseline & UI Shell: implemented.**
**Phase 2 — Audio & Offline Mode ("Plaud" feature): implemented.**
**Phase 4 — Synchronization & MQTT: implemented.**
See [firmware/README.md](firmware/README.md) for build instructions and
architecture notes.

**Phase 3 — Backend API & AI Routing: implemented.**
See [backend/README.md](backend/README.md) for setup, configuration, and
the manual test procedure.

Phase 5 (admin frontend) is not yet started.
