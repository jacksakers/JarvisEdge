"""
Dual-tier AI routing (docs/sdd.txt 2.2, docs/plan.txt Phase 3).

Both tiers talk to Ollama's REST API directly over httpx (async), per
docs/coding.txt 3.1 — never block the event loop on LLM generation.
"""
import logging

import httpx

from app.config import load_config, load_prompts

logger = logging.getLogger(__name__)


async def _ollama_generate(prompt: str, system: str, model: str) -> str:
    cfg = load_config().get("ollama", {})
    host = cfg.get("host", "http://localhost:11434")
    timeout = cfg.get("request_timeout_seconds", 120)

    async with httpx.AsyncClient(timeout=timeout) as client:
        resp = await client.post(f"{host}/api/generate", json={
            "model": model,
            "prompt": prompt,
            "system": system,
            "stream": False,
        })
        resp.raise_for_status()
        return resp.json().get("response", "").strip()


async def fast_reply(transcript: str) -> str:
    """Immediate, lightweight confirmation for the ESP32's Jarvis Feed."""
    cfg = load_config().get("ollama", {})
    prompts = load_prompts()
    return await _ollama_generate(
        transcript, prompts.get("fast_system_prompt", ""), cfg.get("fast_model"),
    )


async def heavy_process(transcript: str) -> str:
    """Batch structuring of a raw transcript into JSON tasks/notes."""
    cfg = load_config().get("ollama", {})
    prompts = load_prompts()
    return await _ollama_generate(
        transcript, prompts.get("heavy_system_prompt", ""), cfg.get("heavy_model"),
    )


async def list_available_models() -> list[str]:
    """Model names Ollama currently has pulled — used to populate the Settings dropdowns."""
    cfg = load_config().get("ollama", {})
    host = cfg.get("host", "http://localhost:11434")
    try:
        async with httpx.AsyncClient(timeout=10) as client:
            resp = await client.get(f"{host}/api/tags")
            resp.raise_for_status()
            return [m["name"] for m in resp.json().get("models", [])]
    except httpx.HTTPError:
        logger.warning("[LLM] Could not reach Ollama at %s to list models.", host)
        return []
