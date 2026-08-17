"""
Device heartbeat tracker.

The ESP32 only otherwise talks to the backend when it has a queued
recording to upload or a Focus item gets tapped, so there's no way to tell
from the Command Center whether the device is even online. This module
holds a tiny in-memory "last seen" record, updated by a periodic
POST /device/heartbeat call from the firmware (see firmware/src/device_heartbeat.cpp).
Not persisted — a backend restart just means "offline" until the next heartbeat.
"""
from datetime import datetime, timezone
from typing import Any, Dict, Optional

# Device is considered offline if no heartbeat has arrived within this window
# (should be a few multiples of the firmware's heartbeat interval).
ONLINE_WINDOW_SECONDS = 60

_last_heartbeat: Optional[Dict[str, Any]] = None


def record_heartbeat(payload: Dict[str, Any]) -> None:
    global _last_heartbeat
    _last_heartbeat = {**payload, "received_at": datetime.now(timezone.utc)}


def get_status() -> Dict[str, Any]:
    if _last_heartbeat is None:
        return {"online": False, "last_seen": None}

    received_at = _last_heartbeat["received_at"]
    age = (datetime.now(timezone.utc) - received_at).total_seconds()
    return {
        "online": age <= ONLINE_WINDOW_SECONDS,
        "last_seen": received_at.isoformat(),
        "seconds_since": round(age),
        "wifi_rssi": _last_heartbeat.get("wifi_rssi"),
        "queue_count": _last_heartbeat.get("queue_count"),
        "firmware": _last_heartbeat.get("firmware"),
    }
