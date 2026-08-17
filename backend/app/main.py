"""
Jarvis Edge Node — Home Server Backend (docs/plan.txt Phase 3)

Ingests WAV recordings uploaded from the ESP32 firmware, transcribes them
locally, routes the text through the fast LLM tier for an immediate reply,
and schedules the heavy LLM tier as a background task. MQTT push-back to
the device (docs/sdd.txt 4.2) is Phase 4 — for now, the fast response is
returned directly in the HTTP response so it can be tested with curl.
"""
import json
import logging
from contextlib import asynccontextmanager

from fastapi import BackgroundTasks, FastAPI, File, HTTPException, UploadFile
from fastapi.responses import JSONResponse
from sqlmodel import select

from app import mqtt
from app.asr import transcribe_wav
from app.config import load_config
from app.database import init_db, session_scope
from app.llm import fast_reply, heavy_process
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
