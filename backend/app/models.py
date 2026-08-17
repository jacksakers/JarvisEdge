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

    # Set when this log's transcript was also handed off to the JARVIS 3.0
    # framework as a delegated Task (see app/jarvis_client.py).
    jarvis_task_id: Optional[int] = None


class FocusItem(SQLModel, table=True):
    """
    One "Daily Focus" to-do item shown on the device's carousel tile.

    Replaces the old MQTT-only, DB-less top-3 list: items are now first-class
    rows so the Command Center frontend gets full CRUD, and the device can
    toggle completion directly over HTTP (see docs/sdd.txt 4.3).
    """

    id: Optional[int] = Field(default=None, primary_key=True)
    created_at: datetime = Field(default_factory=lambda: datetime.now(timezone.utc))

    text: str
    done: bool = False
    # Lower position = shown first. New manual items go to the front.
    position: int = 0
    # "manual" (added via Command Center/device) or "ai" (extracted by the
    # heavy LLM tier from a voice log).
    source: str = "manual"
    # LogEntry.id this item was extracted from, if source == "ai".
    log_entry_id: Optional[int] = None


class ActionEvent(SQLModel, table=True):
    """
    A manual trigger from the device's Action Grid (or the Command Center's
    mirrored quick-action buttons). Kept as history so the frontend has a
    real log to browse instead of the buttons doing nothing.
    """

    id: Optional[int] = Field(default=None, primary_key=True)
    created_at: datetime = Field(default_factory=lambda: datetime.now(timezone.utc))

    # "time_track" | "note" | "alert" | "dismiss"
    action_type: str
    # Free-text payload — the note body for "note", the alert message for
    # "alert", empty for "time_track"/"dismiss".
    text: str = ""
    # Set if this action was also forwarded to the JARVIS 3.0 journal/tasks API.
    jarvis_synced: bool = False
