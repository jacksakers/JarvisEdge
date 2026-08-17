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
from contextlib import asynccontextmanager

from fastapi import BackgroundTasks, FastAPI, File, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from sqlmodel import select

from app import mqtt
from app.asr import transcribe_wav
from app.config import load_config, load_prompts, save_config, save_prompts
from app.database import init_db, session_scope
from app.llm import fast_reply, heavy_process, list_available_models
from app.models import LogEntry

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Reject absurdly large uploads before they hit memory/SD — a few minutes of
# 16-bit mono WAV at 16kHz is well under this.
MAX_UPLOAD_BYTES = 25 * 1024 * 1024

# Matches UI_FOCUS_ITEM_COUNT in firmware/include/ui_screen_focus.h
MAX_FOCUS_ITEMS = 3


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

    # Push the extracted tasks to the ESP32's Daily Focus card (docs/sdd.txt 4.3).
    try:
        tasks = json.loads(structured).get("tasks", [])
    except (json.JSONDecodeError, AttributeError):
        logger.warning("[Heavy] Log %s did not return valid JSON — skipping MQTT push.", log_id)
        return
    cfg = load_config().get("mqtt", {})
    mqtt.publish(cfg.get("topic_focus", "jarvis/ui/focus"), {"tasks": tasks[:MAX_FOCUS_ITEMS]})


@app.post("/upload/audio")
async def upload_audio(background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    """Ingestion endpoint (docs/plan.txt Phase 3.2) — accepts a multipart WAV upload."""
    audio_bytes = await file.read()
    if not audio_bytes:
        raise HTTPException(400, "Empty audio upload")
    if len(audio_bytes) > MAX_UPLOAD_BYTES:
        raise HTTPException(413, "Audio upload too large")

    transcript = transcribe_wav(audio_bytes)
    if not transcript:
        raise HTTPException(422, "Transcription produced no text")

    response_text = await fast_reply(transcript)

    with session_scope() as session:
        entry = LogEntry(raw_text=transcript, fast_response=response_text, status="fast_done")
        session.add(entry)
        session.flush()
        log_id = entry.id

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


# ── Settings / Prompts (Phase 5 — Vite Command Center) ──────────────────────

class SettingsUpdate(BaseModel):
    fast_model: str | None = None
    heavy_model: str | None = None
    mqtt_host: str | None = None
    mqtt_port: int | None = None


class PromptsUpdate(BaseModel):
    fast_system_prompt: str | None = None
    heavy_system_prompt: str | None = None


@app.get("/settings")
async def get_settings():
    """Current Ollama model assignment + MQTT broker config, for the Settings page."""
    cfg = load_config()
    ollama_cfg = cfg.get("ollama", {})
    mqtt_cfg = cfg.get("mqtt", {})
    return {
        "fast_model": ollama_cfg.get("fast_model"),
        "heavy_model": ollama_cfg.get("heavy_model"),
        "mqtt_host": mqtt_cfg.get("host"),
        "mqtt_port": mqtt_cfg.get("port"),
    }


@app.put("/settings")
async def update_settings(update: SettingsUpdate):
    """Apply edits from the Settings page and reconnect MQTT if its config changed."""
    cfg = load_config()
    ollama_cfg = cfg.setdefault("ollama", {})
    mqtt_cfg = cfg.setdefault("mqtt", {})

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
