from datetime import datetime, timezone
from typing import Optional

from sqlmodel import Field, SQLModel


class LogEntry(SQLModel, table=True):
    """One captured-and-transcribed voice note, per docs/sdd.txt 4.2/4.3."""

    id: Optional[int] = Field(default=None, primary_key=True)
    created_at: datetime = Field(default_factory=lambda: datetime.now(timezone.utc))

    raw_text: str                      # Whisper transcription of the uploaded WAV
    fast_response: str = ""            # Fast-tier LLM confirmation (immediate)
    structured_data: Optional[str] = None  # Heavy-tier LLM JSON output (background)

    # pending -> fast_done -> processed (or "failed" if the heavy tier errors)
    status: str = "pending"
