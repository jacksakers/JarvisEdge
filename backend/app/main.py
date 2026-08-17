"""
Jarvis Edge Node — Home Server Backend (docs/plan.txt Phase 3-5)

Ingests WAV recordings uploaded from the ESP32 firmware, transcribes them
locally, routes the text through the fast LLM tier for an immediate reply,
and schedules the heavy LLM tier as a background task. MQTT push-back to
the device (docs/sdd.txt 4.2) pushes the reply/tasks in the background.
Also exposes read/write settings + prompts endpoints for the Phase 5 Vite
admin frontend (../frontend).
"""
import json
import logging
import os
from contextlib import asynccontextmanager
from datetime import datetime, timezone
from typing import Optional

from fastapi import BackgroundTasks, FastAPI, File, HTTPException, UploadFile
from fastapi.concurrency import run_in_threadpool
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel
from sqlmodel import select

from app import jarvis_client, mqtt
from app.asr import transcribe_wav
from app.config import get_audio_dir, load_config, load_prompts, save_config, save_prompts
from app.database import init_db, session_scope
from app.device_status import get_status as get_device_status, record_heartbeat
from app.llm import fast_reply, heavy_process, list_available_models
from app.models import ActionEvent, FocusItem, LogEntry

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Reject absurdly large uploads before they hit memory/SD — a few minutes of
# 16-bit mono WAV at 16kHz is well under this.
MAX_UPLOAD_BYTES = 25 * 1024 * 1024

# Matches UI_FOCUS_ITEM_COUNT in firmware/include/ui_screen_focus.h
MAX_FOCUS_ITEMS = 3


def _save_debug_audio(audio_bytes: bytes, reason: str) -> None:
    """Uploads that fail ASR are otherwise discarded with nothing to inspect —
    stash them so a bad mic capture can actually be listened to/analyzed."""
    debug_dir = get_audio_dir() / "debug_failed"
    debug_dir.mkdir(parents=True, exist_ok=True)
    ts = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S%f")
    path = debug_dir / f"{ts}_{reason}.wav"
    path.write_bytes(audio_bytes)
    logger.warning("[ASR] Saved failed upload for inspection: %s", path)


@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    logger.info("[JarvisEdge Backend] Database ready.")
    mqtt.connect()
    yield
    mqtt.disconnect()


app = FastAPI(title="Jarvis Edge Node — Backend", lifespan=lifespan)

# The Vite admin frontend (Phase 5) runs on a separate dev-server port and,
# once built, may be hosted from a different origin too — this is a
# local-network tool, so allow any origin rather than hardcoding one.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/health")
async def health():
    return {"status": "ok"}

class DeviceHeartbeat(BaseModel):
    wifi_rssi: int | None = None
    queue_count: int | None = None
    firmware: str | None = None

class LogCreate(BaseModel):
    raw_text: str
    fast_response: Optional[str] = ""
    status: Optional[str] = "fast_done"

class LogUpdate(BaseModel):
    raw_text: Optional[str] = None
    fast_response: Optional[str] = None
    status: Optional[str] = None

@app.post("/device/heartbeat")
async def device_heartbeat(payload: DeviceHeartbeat):
    """Called periodically by the firmware (device_heartbeat.cpp) so the
    Command Center can show whether the ESP32 is actually online."""
    record_heartbeat(payload.model_dump())
    return {"ok": True}


@app.get("/device/status")
async def device_status():
    return get_device_status()


def _publish_focus_from_db() -> None:
    """Push the current top undone Focus items to the ESP32 over MQTT."""
    cfg = load_config().get("mqtt", {})
    with session_scope() as session:
        items = session.exec(
            select(FocusItem)
            .where(FocusItem.done == False)  # noqa: E712
            .order_by(FocusItem.position, FocusItem.id)
            .limit(MAX_FOCUS_ITEMS)
        ).all()
        # id is included so the device can POST /focus/{id}/toggle when the
        # user taps a row (see firmware/src/ui_screen_focus.cpp).
        tasks = [{"id": item.id, "text": item.text} for item in items]
    mqtt.publish(cfg.get("topic_focus", "jarvis/ui/focus"), {"tasks": tasks})


async def _run_heavy(log_id: int, transcript: str) -> None:
    """Background task: heavy-tier structuring, run after the HTTP response is sent."""
    try:
        structured = await heavy_process(transcript)
    except Exception:
        logger.exception("[Heavy] Processing failed for log %s", log_id)
        with session_scope() as session:
            entry = session.get(LogEntry, log_id)
            if entry:
                entry.status = "failed"
                session.add(entry)
        return

    with session_scope() as session:
        entry = session.get(LogEntry, log_id)
        if entry:
            entry.structured_data = structured
            entry.status = "processed"
            session.add(entry)
    logger.info("[Heavy] Log %s processed.", log_id)

    # Persist extracted to-dos as real FocusItem rows (full CRUD via the
    # Command Center) and push the current top-3 undone items over MQTT.
    try:
        tasks = json.loads(structured).get("tasks", [])
    except (json.JSONDecodeError, AttributeError):
        logger.warning("[Heavy] Log %s did not return valid JSON — skipping Focus update.", log_id)
        return

    if tasks:
        with session_scope() as session:
            max_pos = session.exec(select(FocusItem)).all()
            next_pos = (max(fi.position for fi in max_pos) + 1) if max_pos else 0
            for offset, task_text in enumerate(tasks):
                session.add(FocusItem(
                    text=task_text, source="ai", log_entry_id=log_id,
                    position=next_pos + offset,
                ))
    _publish_focus_from_db()

    # Optionally hand the same transcript to the full JARVIS 3.0 agent loop
    # (tools + memory) so it can act on it beyond simple task extraction.
    if jarvis_client.is_enabled():
        jarvis_task_id = await jarvis_client.submit_task(transcript)
        if jarvis_task_id is not None:
            with session_scope() as session:
                entry = session.get(LogEntry, log_id)
                if entry:
                    entry.jarvis_task_id = jarvis_task_id
                    session.add(entry)


@app.post("/upload/audio")
async def upload_audio(background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    """Ingestion endpoint (docs/plan.txt Phase 3.2) — accepts a multipart WAV upload."""
    audio_bytes = await file.read()
    if not audio_bytes:
        raise HTTPException(400, "Empty audio upload")
    if len(audio_bytes) > MAX_UPLOAD_BYTES:
        raise HTTPException(413, "Audio upload too large")

    # Whisper inference is CPU-bound and blocking — run off the event loop so
    # it doesn't stall heartbeat/MQTT/other requests for the transcription duration.
    # Any failure here becomes a deterministic 422 (not a bare 500) so the
    # device's sync loop can tell "never going to work" apart from "try again"
    # instead of re-uploading (and re-transcribing) the same file forever.
    try:
        transcript = await run_in_threadpool(transcribe_wav, audio_bytes)
    except Exception:
        logger.exception("[ASR] Transcription raised for a %d-byte upload.", len(audio_bytes))
        _save_debug_audio(audio_bytes, "transcribe-error")
        raise HTTPException(422, "Transcription failed")
    if not transcript:
        _save_debug_audio(audio_bytes, "no-text")
        raise HTTPException(422, "Transcription produced no text")

    try:
        response_text = await fast_reply(transcript)
    except Exception:
        logger.exception("[Fast LLM] fast_reply failed for log transcript.")
        raise HTTPException(422, "Fast-tier response failed")

    # Kept on disk for the Command Center's audio playback (Voice Logs page) —
    # handy for debugging mic/capture issues. Disable via config.yaml `audio.keep_files: false`.
    keep_audio = load_config().get("audio", {}).get("keep_files", True)

    with session_scope() as session:
        entry = LogEntry(raw_text=transcript, fast_response=response_text, status="fast_done")
        session.add(entry)
        session.flush()
        log_id = entry.id
        if keep_audio:
            wav_path = get_audio_dir() / f"log_{log_id}.wav"
            wav_path.write_bytes(audio_bytes)
            entry.audio_path = str(wav_path)
            session.add(entry)

    # Push the immediate confirmation to the ESP32's Jarvis Feed (docs/sdd.txt 4.2).
    cfg = load_config().get("mqtt", {})
    mqtt.publish(cfg.get("topic_feed", "jarvis/ui/feed"), {"text": response_text})

    # Heavy tier runs after the response is returned — the ESP32 can delete
    # its local .wav as soon as it gets this reply (docs/sdd.txt 4.2/4.3).
    background_tasks.add_task(_run_heavy, log_id, transcript)

    return JSONResponse({
        "id": log_id,
        "transcript": transcript,
        "fast_response": response_text,
    })

@app.get("/logs")
async def list_logs(limit: int = 20):
    """Recent log entries — useful for verifying both AI tiers fired (Phase 3 test)."""
    with session_scope() as session:
        entries = session.exec(
            select(LogEntry).order_by(LogEntry.id.desc()).limit(limit)
        ).all()
        return [e.model_dump(mode="json") for e in entries]


@app.post("/logs", status_code=201)
async def create_log(payload: LogCreate):
    with session_scope() as session:
        entry = LogEntry(
            raw_text=payload.raw_text,
            fast_response=payload.fast_response or "",
            status=payload.status or "fast_done",
        )
        session.add(entry)
        session.flush()
        session.refresh(entry)
        return entry.model_dump(mode="json")

@app.patch("/logs/{log_id}")

async def update_log(log_id: int, payload: LogUpdate):
    with session_scope() as session:
        entry = session.get(LogEntry, log_id)
        if not entry:
            raise HTTPException(404, "Log entry not found")

        if payload.raw_text is not None:
            entry.raw_text = payload.raw_text

        if payload.fast_response is not None:
            entry.fast_response = payload.fast_response

        if payload.status is not None:
            entry.status = payload.status

        session.add(entry)
        session.flush()
        session.refresh(entry)
        return entry.model_dump(mode="json")

@app.get("/logs/{log_id}/audio")
async def get_log_audio(log_id: int):
    """Serve the raw WAV for a log entry, for the Voice Logs playback control."""
    with session_scope() as session:
        entry = session.get(LogEntry, log_id)
        if not entry or not entry.audio_path:
            raise HTTPException(404, "No audio saved for this log entry")
        audio_path = entry.audio_path
    if not os.path.exists(audio_path):
        raise HTTPException(404, "Audio file missing on disk")
    return FileResponse(audio_path, media_type="audio/wav", filename=f"log_{log_id}.wav")


def _delete_audio_file(audio_path: Optional[str]) -> None:
    if audio_path and os.path.exists(audio_path):
        try:
            os.remove(audio_path)
        except OSError:
            logger.warning("Could not remove audio file %s", audio_path)


@app.delete("/logs/{log_id}", status_code=204)
async def delete_log(log_id: int):
    with session_scope() as session:
        entry = session.get(LogEntry, log_id)
        if not entry:
            raise HTTPException(404, "Log entry not found")
        _delete_audio_file(entry.audio_path)
        session.delete(entry)


@app.delete("/logs")
async def bulk_delete_logs():
    """Clear the entire voice-log history (Command Center 'Clear History' action)."""
    with session_scope() as session:
        entries = session.exec(select(LogEntry)).all()
        count = len(entries)
        for entry in entries:
            _delete_audio_file(entry.audio_path)
            session.delete(entry)
    return {"deleted": count}


# ── Daily Focus (full CRUD — docs/sdd.txt 4.3) ───────────────────────────────

class FocusCreate(BaseModel):
    text: str


class FocusUpdate(BaseModel):
    text: str | None = None
    done: bool | None = None
    position: int | None = None


@app.get("/focus")
async def list_focus():
    with session_scope() as session:
        items = session.exec(
            select(FocusItem).order_by(FocusItem.position, FocusItem.id)
        ).all()
        return [i.model_dump(mode="json") for i in items]


@app.post("/focus", status_code=201)
async def create_focus(payload: FocusCreate):
    if not payload.text.strip():
        raise HTTPException(422, "text is required")
    with session_scope() as session:
        existing = session.exec(select(FocusItem)).all()
        # New manual items go to the front of the list.
        min_pos = min((i.position for i in existing), default=0)
        item = FocusItem(text=payload.text.strip(), source="manual", position=min_pos - 1)
        session.add(item)
        session.flush()
        session.refresh(item)
        result = item.model_dump(mode="json")
    _publish_focus_from_db()
    return result


@app.patch("/focus/{item_id}")
async def update_focus(item_id: int, payload: FocusUpdate):
    with session_scope() as session:
        item = session.get(FocusItem, item_id)
        if not item:
            raise HTTPException(404, "Focus item not found")
        if payload.text is not None:
            item.text = payload.text.strip()
        if payload.done is not None:
            item.done = payload.done
        if payload.position is not None:
            item.position = payload.position
        session.add(item)
        session.flush()
        session.refresh(item)
        result = item.model_dump(mode="json")
    _publish_focus_from_db()
    return result


@app.post("/focus/{item_id}/toggle")
async def toggle_focus(item_id: int):
    """Convenience endpoint for the device — tapping a Daily Focus row calls this."""
    with session_scope() as session:
        item = session.get(FocusItem, item_id)
        if not item:
            raise HTTPException(404, "Focus item not found")
        item.done = not item.done
        session.add(item)
        session.flush()
        session.refresh(item)
        result = item.model_dump(mode="json")
    _publish_focus_from_db()
    return result


@app.delete("/focus/{item_id}", status_code=204)
async def delete_focus(item_id: int):
    with session_scope() as session:
        item = session.get(FocusItem, item_id)
        if not item:
            raise HTTPException(404, "Focus item not found")
        session.delete(item)
    _publish_focus_from_db()


# ── Action Grid (docs/sdd.txt 3 — mirrored by the device's Action Grid tile
#    and the Command Center's quick-action buttons) ──────────────────────────

VALID_ACTION_TYPES = {"time_track", "note", "alert", "dismiss"}


class ActionPayload(BaseModel):
    text: str | None = None


@app.post("/actions/{action_type}")
async def trigger_action(action_type: str, payload: ActionPayload = ActionPayload()):
    if action_type not in VALID_ACTION_TYPES:
        raise HTTPException(404, f"Unknown action '{action_type}'")

    text = (payload.text or "").strip()
    cfg = load_config().get("mqtt", {})
    feed_topic = cfg.get("topic_feed", "jarvis/ui/feed")
    jarvis_synced = False

    if action_type == "time_track":
        now = datetime.now(timezone.utc).astimezone()
        feed_text = f"Time tracked at {now.strftime('%H:%M')}."
        mqtt.publish(feed_topic, {"text": feed_text})

    elif action_type == "note":
        if not text:
            raise HTTPException(422, "text is required for a note")
        mqtt.publish(feed_topic, {"text": "Note saved."})
        if jarvis_client.is_enabled():
            jarvis_synced = await jarvis_client.submit_journal_note("Edge Node Note", text)

    elif action_type == "alert":
        if not text:
            raise HTTPException(422, "text is required for an alert")
        mqtt.publish(feed_topic, {"text": f"\u26a0 {text}"})

    elif action_type == "dismiss":
        mqtt.publish(feed_topic, {"text": "Jarvis is ready."})

    with session_scope() as session:
        event = ActionEvent(action_type=action_type, text=text, jarvis_synced=jarvis_synced)
        session.add(event)
        session.flush()
        session.refresh(event)
        result = event.model_dump(mode="json")
    return result


@app.get("/actions")
async def list_actions(limit: int = 50):
    with session_scope() as session:
        events = session.exec(
            select(ActionEvent).order_by(ActionEvent.id.desc()).limit(limit)
        ).all()
        return [e.model_dump(mode="json") for e in events]


@app.delete("/actions/{event_id}", status_code=204)
async def delete_action(event_id: int):
    with session_scope() as session:
        event = session.get(ActionEvent, event_id)
        if not event:
            raise HTTPException(404, "Action event not found")
        session.delete(event)


@app.delete("/actions")
async def bulk_delete_actions():
    with session_scope() as session:
        events = session.exec(select(ActionEvent)).all()
        count = len(events)
        for event in events:
            session.delete(event)
    return {"deleted": count}


# ── JARVIS 3.0 integration (optional — app/jarvis_client.py) ─────────────────

@app.get("/jarvis/status")
async def jarvis_status():
    cfg = load_config().get("jarvis", {})
    if not cfg.get("enabled"):
        return {"enabled": False, "connected": False}
    check = await jarvis_client.check_connection()
    return {"enabled": True, **check}


@app.get("/jarvis/feed")
async def jarvis_feed(limit: int = 20):
    """Read-only mirror of the JARVIS 3.0 feed, for the Command Center's JARVIS tab."""
    return await jarvis_client.get_recent_feed(limit)


# ── Settings / Prompts (Phase 5 — Vite Command Center) ──────────────────────

class SettingsUpdate(BaseModel):

    fast_model: str | None = None
    heavy_model: str | None = None
    mqtt_host: str | None = None
    mqtt_port: int | None = None
    jarvis_enabled: bool | None = None
    jarvis_base_url: str | None = None

    ambient_vad_mode: bool | None = None
    power_saving_mode: bool | None = None
    screen_off_timeout: int | None = None
    screen_lock_enabled: bool | None = None

class PromptsUpdate(BaseModel):
    fast_system_prompt: str | None = None
    heavy_system_prompt: str | None = None

@app.get("/settings")
async def get_settings():
    """Current Ollama model assignment + MQTT broker config, for the Settings page."""
    cfg = load_config()
    ollama_cfg = cfg.get("ollama", {})
    mqtt_cfg = cfg.get("mqtt", {})
    jarvis_cfg = cfg.get("jarvis", {})
    device_cfg = cfg.get("device", {})
    return {
        "fast_model": ollama_cfg.get("fast_model"),
        "heavy_model": ollama_cfg.get("heavy_model"),
        "mqtt_host": mqtt_cfg.get("host"),
        "mqtt_port": mqtt_cfg.get("port"),
        "jarvis_enabled": jarvis_cfg.get("enabled", False),
        "jarvis_base_url": jarvis_cfg.get("base_url", ""),

        "ambient_vad_mode": device_cfg.get("ambient_vad_mode", False),
        "power_saving_mode": device_cfg.get("power_saving_mode", False),
        "screen_off_timeout": device_cfg.get("screen_off_timeout", 30),
        "screen_lock_enabled": device_cfg.get("screen_lock_enabled", False),
    }


@app.put("/settings")
async def update_settings(update: SettingsUpdate):
    """Apply edits from the Settings page and reconnect MQTT if its config changed."""
    cfg = load_config()
    ollama_cfg = cfg.setdefault("ollama", {})
    mqtt_cfg = cfg.setdefault("mqtt", {})
    jarvis_cfg = cfg.setdefault("jarvis", {})
    device_cfg = cfg.setdefault("device", {})

    if update.fast_model is not None:
        ollama_cfg["fast_model"] = update.fast_model

    if update.heavy_model is not None:
        ollama_cfg["heavy_model"] = update.heavy_model

    mqtt_changed = False

    if update.mqtt_host is not None and update.mqtt_host != mqtt_cfg.get("host"):
        mqtt_cfg["host"] = update.mqtt_host
        mqtt_changed = True

    if update.mqtt_port is not None and update.mqtt_port != mqtt_cfg.get("port"):
        mqtt_cfg["port"] = update.mqtt_port
        mqtt_changed = True

    if update.jarvis_enabled is not None:
        jarvis_cfg["enabled"] = update.jarvis_enabled

    if update.jarvis_base_url is not None:
        jarvis_cfg["base_url"] = update.jarvis_base_url

    if update.ambient_vad_mode is not None:
        device_cfg["ambient_vad_mode"] = update.ambient_vad_mode

    if update.power_saving_mode is not None:
        device_cfg["power_saving_mode"] = update.power_saving_mode

    if update.screen_off_timeout is not None:
        device_cfg["screen_off_timeout"] = update.screen_off_timeout

    if update.screen_lock_enabled is not None:
        device_cfg["screen_lock_enabled"] = update.screen_lock_enabled

    save_config(cfg)

    if mqtt_changed:
        mqtt.disconnect()
        mqtt.connect()

    return await get_settings()


@app.get("/models")
async def get_models():
    """Model names Ollama currently has pulled — populates the Settings dropdowns."""
    return {"models": await list_available_models()}


@app.get("/prompts")
async def get_prompts():
    prompts = load_prompts()
    return {
        "fast_system_prompt": prompts.get("fast_system_prompt", ""),
        "heavy_system_prompt": prompts.get("heavy_system_prompt", ""),
    }


@app.put("/prompts")
async def update_prompts(update: PromptsUpdate):
    prompts = load_prompts()
    if update.fast_system_prompt is not None:
        prompts["fast_system_prompt"] = update.fast_system_prompt
    if update.heavy_system_prompt is not None:
        prompts["heavy_system_prompt"] = update.heavy_system_prompt
    save_prompts(prompts)
    return await get_prompts()
