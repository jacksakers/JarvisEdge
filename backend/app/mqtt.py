"""
MQTT publisher (docs/plan.txt Phase 4 — "Backend Publisher").

A single persistent paho-mqtt client connects at startup and pushes JSON
UI-update payloads to the ESP32 over Mosquitto. Publishing is fire-and-forget
and a no-op if no broker is configured/reachable, so the rest of the API
keeps working even without MQTT set up.
"""
import json
import logging
from typing import Any, Dict, Optional

import paho.mqtt.client as mqtt

from app.config import load_config

logger = logging.getLogger(__name__)

_client: Optional[mqtt.Client] = None


def _on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        logger.info("[MQTT] Connected to broker.")
    else:
        logger.warning("[MQTT] Connect failed: %s", reason_code)


def connect() -> None:
    """Start the persistent MQTT connection. No-op if unconfigured/unreachable."""
    global _client
    cfg = load_config().get("mqtt", {})
    host = cfg.get("host")
    if not host:
        logger.info("[MQTT] No broker configured — UI push-back disabled.")
        return

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="jarvis-edge-backend")
    client.on_connect = _on_connect
    try:
        client.connect(host, cfg.get("port", 1883), keepalive=30)
        client.loop_start()
        _client = client
    except OSError as exc:
        logger.warning("[MQTT] Could not connect to %s:%s — %s", host, cfg.get("port", 1883), exc)


def disconnect() -> None:
    global _client
    if _client:
        _client.loop_stop()
        _client.disconnect()
        _client = None


def publish(topic: str, payload: Dict[str, Any]) -> None:
    """Fire-and-forget JSON publish. No-op if the broker isn't connected."""
    if not _client:
        return
    try:
        _client.publish(topic, json.dumps(payload), qos=0)
    except Exception:
        logger.exception("[MQTT] Publish to %s failed", topic)
