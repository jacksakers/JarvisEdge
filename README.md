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
```

Future phases will add a FastAPI backend and a Vite admin frontend per
[docs/plan.txt](docs/plan.txt) Phases 3-5; those will live in sibling
directories here once started.

## Status

**Phase 1 — Hardware Baseline & UI Shell: implemented.** See
[firmware/README.md](firmware/README.md) for build instructions and
architecture notes.

Phases 2-5 (offline audio queue, backend API, MQTT sync, admin frontend) are
not yet started.
