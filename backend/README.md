# Jarvis Edge Node — Backend

FastAPI service implementing **Phase 3** and **Phase 4** of
[../docs/plan.txt](../docs/plan.txt): audio ingestion, local transcription,
dual-tier LLM routing, and MQTT push-back so the ESP32 can display AI
responses without polling.

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
                                 └──> app/mqtt.py publish("jarvis/ui/focus", {"tasks": [...]})
```

See [../docs/sdd.txt](../docs/sdd.txt) sections 2.2/4.2/4.3 for the full
system design this implements.

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
  ASR model size/device, and the `mqtt` section (broker host/port + topic
  names).
- `prompts.yaml` — system prompts for the fast and heavy LLM tiers (kept out
  of Python source per [../docs/coding.txt](../docs/coding.txt) 3.2 so the
  Vite frontend can edit them in Phase 5).

### MQTT push-back (Phase 4)

The backend publishes UI updates via [Mosquitto](https://mosquitto.org/) (or
any MQTT broker) so the ESP32 can update its screen the moment a response is
ready, instead of polling `/logs`:

| Topic              | Payload                                              | Published when                          |
|---------------------|--------------------------------------------------------|------------------------------------------|
| `jarvis/ui/feed`    | `{"text": "..."}`                                     | Right after the fast-tier reply is ready, and on every action-grid trigger |
| `jarvis/ui/focus`   | `{"tasks": [{"id": 1, "text": "..."}, ...]}` (up to 3) | After the heavy tier finishes, and after any Focus CRUD mutation (create/update/toggle/delete) |

The `id` field in each focus task lets the device echo a tap straight back
to `POST /focus/{id}/toggle` (see `firmware/include/edge_api.h`) so a manual
toggle on the device is reflected on the Command Center too.

Leave `mqtt.host` blank in `config.yaml` to disable this entirely —
`/upload/audio` still works and simply won't notify the ESP32. If the broker
is unreachable at startup, `app/mqtt.py` logs a warning and every `publish()`
call becomes a silent no-op; it never blocks or crashes request handling.

Install a local broker for testing:

```bash
sudo apt-get install mosquitto mosquitto-clients
mosquitto_sub -h localhost -t 'jarvis/ui/#' -v   # watch published messages
```

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
| GET    | `/settings`     | Current `{fast_model, heavy_model, mqtt_host, mqtt_port}`.       |
| PUT    | `/settings`     | Update any subset of the above; rewrites `config.yaml` and reconnects MQTT if the broker config changed. |
| GET    | `/models`       | Model names currently pulled in Ollama (`GET /api/tags`), for populating dropdowns. Returns `{"models": []}` if Ollama is unreachable. |
| GET    | `/prompts`      | Current `{fast_system_prompt, heavy_system_prompt}`.             |
| PUT    | `/prompts`      | Update either prompt; rewrites `prompts.yaml`.                   |
| \*     | `/focus*`       | Full CRUD for Daily Focus items — see below.                     |
| \*     | `/actions*`     | Trigger and browse Action Grid events — see below.               |
| GET    | `/jarvis/*`     | JARVIS 3.0 integration status/feed mirror — see below.           |

These endpoints power the Vite Command Center frontend in
[../frontend](../frontend) and are also safe to call directly with `curl`.
CORS is wide open (`allow_origins=["*"]`) since this is a local-network-only
admin tool. `save_config()`/`save_prompts()` in `app/config.py` rewrite the
YAML files with `yaml.safe_dump` — comments in `config.yaml`/`prompts.yaml`
are lost the first time either file is edited via the API.

### Daily Focus (full CRUD)

| Method | Path                  | Purpose                                                          |
|--------|-----------------------|--------------------------------------------------------------------|
| GET    | `/focus`              | All `FocusItem` rows, ordered by position.                         |
| POST   | `/focus`              | Create a manual item (`{"text": "..."}`); inserted at the front.    |
| PATCH  | `/focus/{id}`         | Update `text` and/or `done`.                                        |
| POST   | `/focus/{id}/toggle`  | Flip `done`. Used by the device's tap-to-complete gesture.          |
| DELETE | `/focus/{id}`         | Delete one item.                                                    |

Every mutation republishes the top 3 undone items to `jarvis/ui/focus` so the
device's Daily Focus tile stays in sync without polling.

### Action Grid

| Method | Path                     | Purpose                                                                 |
|--------|--------------------------|---------------------------------------------------------------------------|
| POST   | `/actions/{action_type}` | Trigger `time_track`, `note`, `alert`, or `dismiss`. `note`/`alert` require `{"text": "..."}`. |
| GET    | `/actions`               | Recent `ActionEvent` rows (`?limit=`).                                     |
| DELETE | `/actions/{id}`          | Delete one event.                                                          |
| DELETE | `/actions`               | Clear all action history.                                                  |

`note` events are additionally forwarded to JARVIS 3.0's journal endpoint
(best-effort) when the JARVIS integration is enabled.

### JARVIS 3.0 integration

| Method | Path              | Purpose                                                              |
|--------|-------------------|--------------------------------------------------------------------------|
| GET    | `/jarvis/status`  | `{"enabled": bool, "connected": bool}` — checks `jarvis.base_url/health`. |
| GET    | `/jarvis/feed`    | Read-only mirror of the JARVIS 3.0 feed (`?limit=`).                      |

Controlled by the `jarvis` section of `config.yaml` (`enabled`, `base_url`,
`api_prefix`) and editable from the Command Center's Settings page. All calls
in `app/jarvis_client.py` are best-effort — if JARVIS 3.0 is offline, the
Edge Node keeps working standalone.

## Manual test (Phase 3 acceptance)

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
- `FocusItem` — Daily Focus rows: `text`, `done`, `position`, `source`
  (`manual` or `ai` — AI-extracted tasks come from the heavy tier's JSON).
- `ActionEvent` — history of Action Grid triggers: `action_type`, `text`,
  `jarvis_synced` (whether it was also forwarded to JARVIS 3.0's journal).
