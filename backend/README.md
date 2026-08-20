# House Phone — Backend

FastAPI service implementing the Jarvis dual-tier AI pipeline (audio
ingestion, local transcription, LLM routing, MQTT push-back) plus Ambient
Home Tapo bulb control for the House Phone pivot (see
[../docs/sdd.txt](../docs/sdd.txt)).

## Architecture

```
ESP32 --(multipart WAV)--> POST /upload/audio
                                 │
                                 ▼
                        app/asr.py (faster-whisper, local, CPU)
                                 │  transcript
                                 ▼
                        app/llm.py fast_reply()  ──> Ollama fast_model
                                 │  (awaited — included in the HTTP response)
                                 ▼
                     LogEntry row saved (status="fast_done")
                                 │
                                 ├──> app/mqtt.py publish("jarvis/ui/feed", {"text": ...})
                                 ▼
              BackgroundTasks ──> app/llm.py heavy_process() ──> Ollama heavy_model
                                 │  (runs after the response is sent)
                                 ▼
                     LogEntry row updated (status="processed")
                                 │
                                 └──> extracted to-dos saved as FocusItem rows (Command Center's Todo List)

ESP32 --(fire-and-forget POST)--> /tapo/zones/{id}/toggle | /brightness
ESP32 <--(periodic GET)---------- /tapo/zones                              app/tapo.py (python-kasa, KLAP protocol) <──> Tapo bulb LAN IP
```

See [../docs/sdd.txt](../docs/sdd.txt) for the full system design this
implements.

## Setup

```bash
cd backend
python3 -m venv .venv
./.venv/bin/pip install -r requirements.txt
```

Requires a running [Ollama](https://ollama.com) instance with the models
named in `config.yaml` (`ollama.fast_model` / `ollama.heavy_model`) pulled
locally. The first `/upload/audio` call also downloads the faster-whisper
model weights (`asr.model_size` in `config.yaml`) from Hugging Face.

## Configuration

- `config.yaml` — server host/port, SQLite path, Ollama host + model names,
  ASR model size/device, `mqtt` (broker host/port + feed topic), and `tapo`
  (Tapo account email/password for Ambient Home).
- `prompts.yaml` — system prompts for the fast and heavy LLM tiers (kept out
  of Python source per [../docs/coding.txt](../docs/coding.txt) 3.2 so the
  Vite frontend can edit them).

### MQTT push-back

The backend publishes the fast-tier reply via [Mosquitto](https://mosquitto.org/)
(or any MQTT broker) so the ESP32 can update its Jarvis Voice Capture tile
the moment a response is ready, instead of polling `/logs`:

| Topic              | Payload            | Published when                          |
|---------------------|--------------------|------------------------------------------|
| `jarvis/ui/feed`    | `{"text": "..."}`  | Right after the fast-tier reply is ready |

Leave `mqtt.host` blank in `config.yaml` to disable this entirely —
`/upload/audio` still works and simply won't notify the ESP32. If the broker
is unreachable at startup, `app/mqtt.py` logs a warning and every `publish()`
call becomes a silent no-op; it never blocks or crashes request handling.

Install a local broker for testing:

```bash
sudo apt-get install mosquitto mosquitto-clients
mosquitto_sub -h localhost -t 'jarvis/ui/#' -v   # watch published messages
```

### Ambient Home (Tapo bulb control)

`app/tapo.py` wraps [`python-kasa`](https://github.com/python-kasa/python-kasa)
to speak Tapo's local KLAP protocol directly to each bulb's LAN IP — this
lives on the backend rather than the ESP32 so the tricky auth/encryption
handshake stays in a maintained library instead of hand-rolled firmware
crypto (docs/coding.txt 3.3). Set your Tapo account email/password (same
login as the Tapo app) via `PUT /settings` (`tapo_email`/`tapo_password`,
editable from the Command Center's Settings page) — it's only ever used for
the local handshake with each bulb's own IP. Every call in `app/tapo.py` is
best-effort: an unreachable bulb returns `{"reachable": false}` instead of
raising, so one dead bulb can't take down the whole Ambient Home grid.

## Running

```bash
python run.py
```

Starts uvicorn on the host/port from `config.yaml` (default
`0.0.0.0:8010`).

## Endpoints

| Method | Path            | Purpose                                                        |
|--------|-----------------|-----------------------------------------------------------------|
| GET    | `/health`       | Liveness check.                                                 |
| POST   | `/upload/audio` | Multipart WAV upload. Returns `{id, transcript, fast_response}`. |
| GET    | `/logs`         | Recent `LogEntry` rows (verify both AI tiers fired).             |
| GET    | `/settings`     | Current Ollama/MQTT/JARVIS/Tapo/device config.                  |
| PUT    | `/settings`     | Update any subset of the above; rewrites `config.yaml` and reconnects MQTT if the broker config changed. |
| GET    | `/models`       | Model names currently pulled in Ollama (`GET /api/tags`), for populating dropdowns. Returns `{"models": []}` if Ollama is unreachable. |
| GET    | `/prompts`      | Current `{fast_system_prompt, heavy_system_prompt}`.             |
| PUT    | `/prompts`      | Update either prompt; rewrites `prompts.yaml`.                   |
| \*     | `/focus*`       | Full CRUD for the Todo List — see below.                         |
| \*     | `/tapo/zones*`  | Ambient Home Tapo zone CRUD + control — see below.               |
| GET    | `/jarvis/*`     | JARVIS 3.0 integration status/feed mirror — see below.           |

These endpoints power the Vite Command Center frontend in
[../frontend](../frontend) and are also safe to call directly with `curl`.
CORS is wide open (`allow_origins=["*"]`) since this is a local-network-only
admin tool. `save_config()`/`save_prompts()` in `app/config.py` rewrite the
YAML files with `yaml.safe_dump` — comments in `config.yaml`/`prompts.yaml`
are lost the first time either file is edited via the API.

### Todo List (full CRUD, Command Center only)

| Method | Path                  | Purpose                                                          |
|--------|-----------------------|--------------------------------------------------------------------|
| GET    | `/focus`              | All `FocusItem` rows, ordered by position.                         |
| POST   | `/focus`              | Create a manual item (`{"text": "..."}`); inserted at the front.    |
| PATCH  | `/focus/{id}`         | Update `text` and/or `done`.                                        |
| POST   | `/focus/{id}/toggle`  | Flip `done`.                                                        |
| DELETE | `/focus/{id}`         | Delete one item.                                                    |

There is no on-device Todo List tile — voice notes that the heavy LLM tier
extracts tasks from land here (`source="ai"`), otherwise it's managed
entirely from the Command Center.

### Ambient Home (Tapo)

| Method | Path                          | Purpose                                                          |
|--------|-------------------------------|----------------------------------------------------------------------|
| GET    | `/tapo/zones`                 | All zones, live-polled in parallel (`{"reachable": bool, "on": bool, "brightness": int}` merged in). |
| POST   | `/tapo/zones`                 | Create a zone (`{"name", "room", "ip"}`).                             |
| PATCH  | `/tapo/zones/{id}`            | Update name/room/ip.                                                  |
| DELETE | `/tapo/zones/{id}`            | Delete a zone.                                                        |
| POST   | `/tapo/zones/{id}/toggle`     | Flip the bulb's actual current on/off state.                          |
| POST   | `/tapo/zones/{id}/brightness` | Set brightness 1-100 (also turns the bulb on).                        |
| POST   | `/tapo/zones/all_off`         | Turn every configured zone off.                                       |

### JARVIS 3.0 integration

| Method | Path              | Purpose                                                              |
|--------|-------------------|--------------------------------------------------------------------------|
| GET    | `/jarvis/status`  | `{"enabled": bool, "connected": bool}` — checks `jarvis.base_url/health`. |
| GET    | `/jarvis/feed`    | Read-only mirror of the JARVIS 3.0 feed (`?limit=`).                      |

Controlled by the `jarvis` section of `config.yaml` (`enabled`, `base_url`,
`api_prefix`) and editable from the Command Center's Settings page. All calls
in `app/jarvis_client.py` are best-effort — if JARVIS 3.0 is offline, the
backend keeps working standalone.

## Manual test (voice capture pipeline)

```bash
curl -X POST http://localhost:8010/upload/audio \
  -F "file=@/path/to/queue/log_0.wav;type=audio/wav"

curl http://localhost:8010/logs
```

The first call returns the transcript + fast-tier reply immediately; polling
`/logs` a few seconds later should show `status: "processed"` with
`structured_data` populated once the heavy tier finishes in the background.

## Data

SQLite database (`jarvis_edge.db` by default) holds:

- `LogEntry` — one row per voice upload: `raw_text` (transcript),
  `fast_response`, `structured_data` (heavy tier JSON), `status`
  (`fast_done` -> `processed`/`failed`), and `jarvis_task_id` if delegated.
- `FocusItem` — Todo List rows: `text`, `done`, `position`, `source`
  (`manual` or `ai` — AI-extracted tasks come from the heavy tier's JSON).
- `TapoZone` — Ambient Home zones: `name`, `room`, `ip`, and last-known
  `on`/`brightness` (the live state is always re-polled from the bulb).

