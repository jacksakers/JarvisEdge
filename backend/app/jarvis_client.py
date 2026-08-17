"""
JARVIS 3.0 integration client (docs/sdd.txt — Edge Node as a capture point
for the full "Jarvis" home AI framework, not just the local dual-tier model).

Optional and best-effort: every call is fire-and-forget with a short timeout
and swallows connection errors, so the Edge Node backend keeps working at
full speed whether or not a JARVIS 3.0 instance is reachable. Enable it by
setting `jarvis.enabled: true` and `jarvis.base_url` in config.yaml.

Integration points used:
  POST {base}/api/v1/tasks/            — delegate a voice transcript to the
                                          full JARVIS agent loop (tools, memory)
  POST {base}/api/v1/journal/          — forward a manual "Note" action into
                                          the JARVIS journal
  GET  {base}/api/v1/feed/             — pull recent JARVIS feed items (for
                                          the Command Center's JARVIS tab)
  GET  {base}/health                   — connectivity check for Settings
"""
import logging
from typing import Any, Dict, List, Optional

import httpx

from app.config import load_config

logger = logging.getLogger(__name__)

_TIMEOUT = 5.0


def _jarvis_cfg() -> Dict[str, Any]:
    return load_config().get("jarvis", {})


def is_enabled() -> bool:
    return bool(_jarvis_cfg().get("enabled"))


def _base_url() -> str:
    cfg = _jarvis_cfg()
    base = cfg.get("base_url", "http://localhost:8000").rstrip("/")
    prefix = cfg.get("api_prefix", "/api/v1")
    return f"{base}{prefix}"


async def check_connection() -> Dict[str, Any]:
    """Used by the Settings page's "Test Connection" button."""
    cfg = _jarvis_cfg()
    base = cfg.get("base_url", "http://localhost:8000").rstrip("/")
    try:
        async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
            resp = await client.get(f"{base}/health")
            resp.raise_for_status()
            return {"connected": True, "detail": resp.json()}
    except httpx.HTTPError as exc:
        return {"connected": False, "detail": str(exc)}


async def submit_task(prompt: str) -> Optional[int]:
    """Delegate a transcript to the full JARVIS agent loop. Returns the task id."""
    if not is_enabled():
        return None
    try:
        async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
            resp = await client.post(f"{_base_url()}/tasks/", json={"prompt": prompt})
            resp.raise_for_status()
            return resp.json().get("id")
    except httpx.HTTPError as exc:
        logger.warning("[JARVIS] submit_task failed: %s", exc)
        return None


async def submit_journal_note(title: str, content: str) -> bool:
    """Forward a manual "Note" action into the JARVIS journal."""
    if not is_enabled():
        return False
    try:
        async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
            resp = await client.post(
                f"{_base_url()}/journal/",
                json={"title": title, "content": content},
            )
            resp.raise_for_status()
            return True
    except httpx.HTTPError as exc:
        logger.warning("[JARVIS] submit_journal_note failed: %s", exc)
        return False


async def get_recent_feed(limit: int = 20) -> List[Dict[str, Any]]:
    """Read-only mirror of the JARVIS feed, for the Command Center's JARVIS tab."""
    if not is_enabled():
        return []
    try:
        async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
            resp = await client.get(f"{_base_url()}/feed/", params={"limit": limit})
            resp.raise_for_status()
            return resp.json()
    except httpx.HTTPError as exc:
        logger.warning("[JARVIS] get_recent_feed failed: %s", exc)
        return []
