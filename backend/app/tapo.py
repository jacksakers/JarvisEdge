"""
Tapo smart-bulb control (docs/sdd.txt Card 1 — "Ambient Home").

The House Phone's Ambient Home grid controls TP-Link Tapo bulbs directly
over the LAN using the KLAP local protocol. That handshake (auth + AES
encryption) is deliberately NOT reimplemented in firmware — it's routed
through this backend module and the actively-maintained `python-kasa`
library instead, so the ESP32 only ever speaks plain HTTP to a server we
control (docs/coding.txt 2.1 — keep the edge device "dumb").

Requires a Tapo account email/password in config.yaml (`tapo.email` /
`tapo.password`) — the same login used in the Tapo app. It's only ever used
for the local KLAP handshake with each bulb's own IP; it is never sent
anywhere else.

Every call here is best-effort: a bulb that's unplugged, off-network, or
misconfigured must never take down the whole Ambient Home grid, so failures
are logged and turned into a `{"reachable": False}`-shaped result instead of
a raised exception wherever the caller needs one.
"""
import logging
from typing import Any, Dict

from app.config import load_config

logger = logging.getLogger(__name__)


def _credentials():
    from kasa import Credentials  # imported lazily — optional/heavy dependency

    cfg = load_config().get("tapo", {})
    return Credentials(cfg.get("email", ""), cfg.get("password", ""))


async def _connect(ip: str):
    from kasa import Discover  # imported lazily — optional/heavy dependency

    # discover_single talks directly to a known host (no network-wide scan)
    # and auto-negotiates the right protocol/encryption for the device.
    return await Discover.discover_single(ip, credentials=_credentials())


async def get_zone_state(ip: str) -> Dict[str, Any]:
    """Live state read for one bulb. Returns reachable=False on any failure."""
    try:
        dev = await _connect(ip)
        await dev.update()
        return {
            "reachable": True,
            "on": bool(dev.is_on),
            "brightness": getattr(dev, "brightness", None),
        }
    except Exception as exc:
        logger.warning("[Tapo] %s unreachable: %s", ip, exc)
        return {"reachable": False, "on": False, "brightness": None}


async def set_power(ip: str, on: bool) -> bool:
    try:
        dev = await _connect(ip)
        await (dev.turn_on() if on else dev.turn_off())
        return True
    except Exception as exc:
        logger.warning("[Tapo] set_power(%s, %s) failed: %s", ip, on, exc)
        return False


async def set_brightness(ip: str, brightness: int) -> bool:
    """Also turns the bulb on — dragging a brightness slider implies "on"."""
    try:
        dev = await _connect(ip)
        if not dev.is_on:
            await dev.turn_on()
        await dev.set_brightness(max(1, min(100, brightness)))
        return True
    except Exception as exc:
        logger.warning("[Tapo] set_brightness(%s, %s) failed: %s", ip, brightness, exc)
        return False
