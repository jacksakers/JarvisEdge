import os
from pathlib import Path
from typing import Any, Dict

import yaml

_HERE = Path(__file__).parent.parent.resolve()
_CONFIG_PATH = _HERE / "config.yaml"
_PROMPTS_PATH = _HERE / "prompts.yaml"

_config_cache: Dict[str, Any] = {}
_prompts_cache: Dict[str, Any] = {}


def _load_yaml(path: Path, cache: Dict[str, Any]) -> Dict[str, Any]:
    if cache:
        return cache
    try:
        with open(path) as f:
            cache.update(yaml.safe_load(f) or {})
    except FileNotFoundError:
        print(f"[Config] Warning: {path} not found. Using defaults.")
    except yaml.YAMLError as exc:
        print(f"[Config] Error parsing {path}: {exc}. Using defaults.")
    return cache


def load_config() -> Dict[str, Any]:
    """Load config.yaml and return it as a dict. Result is cached."""
    return _load_yaml(_CONFIG_PATH, _config_cache)


def load_prompts() -> Dict[str, Any]:
    """Load prompts.yaml (system prompts for the fast/heavy LLM tiers)."""
    return _load_yaml(_PROMPTS_PATH, _prompts_cache)


def save_config(cfg: Dict[str, Any]) -> None:
    """Persist the full config dict back to config.yaml and refresh the cache."""
    with open(_CONFIG_PATH, "w") as f:
        yaml.safe_dump(cfg, f, sort_keys=False)
    _config_cache.clear()
    _config_cache.update(cfg)


def save_prompts(prompts: Dict[str, Any]) -> None:
    """Persist the full prompts dict back to prompts.yaml and refresh the cache."""
    with open(_PROMPTS_PATH, "w") as f:
        yaml.safe_dump(prompts, f, sort_keys=False)
    _prompts_cache.clear()
    _prompts_cache.update(prompts)


def get_db_path() -> str:
    cfg = load_config()
    db_path = cfg.get("database", {}).get("path", "jarvis_edge.db")
    if not os.path.isabs(db_path):
        return str(_HERE / db_path)
    return db_path


def get_audio_dir() -> Path:
    """Directory where uploaded WAVs are kept for playback (created on first use)."""
    cfg = load_config()
    audio_path = cfg.get("audio", {}).get("path", "audio_logs")
    path = Path(audio_path) if os.path.isabs(audio_path) else (_HERE / audio_path)
    path.mkdir(parents=True, exist_ok=True)
    return path
