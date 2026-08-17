"""
Local ASR transcription (docs/plan.txt Phase 3 — "Transcription Pipeline").

Uses faster-whisper (CTranslate2) so it runs entirely on the home server
CPU/GPU with no external API calls. The model is loaded lazily and kept
as a module-level singleton so repeated uploads don't reload weights.
"""
import logging
import tempfile

from app.config import load_config

logger = logging.getLogger(__name__)

_model = None


def _get_model():
    global _model
    if _model is None:
        from faster_whisper import WhisperModel  # imported lazily — heavy dependency

        cfg = load_config().get("asr", {})
        logger.info("[ASR] Loading faster-whisper model '%s' (%s/%s)...",
                    cfg.get("model_size", "base"), cfg.get("device", "cpu"),
                    cfg.get("compute_type", "int8"))
        _model = WhisperModel(
            cfg.get("model_size", "base"),
            device=cfg.get("device", "cpu"),
            compute_type=cfg.get("compute_type", "int8"),
        )
    return _model


def transcribe_wav(audio_bytes: bytes) -> str:
    """Transcribe raw WAV bytes (as uploaded from the ESP32) to text."""
    model = _get_model()
    with tempfile.NamedTemporaryFile(suffix=".wav") as tmp:
        tmp.write(audio_bytes)
        tmp.flush()
        segments, _info = model.transcribe(tmp.name)
        return " ".join(seg.text.strip() for seg in segments).strip()
