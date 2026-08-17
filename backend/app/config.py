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


def get_db_path() -> str:
    cfg = load_config()
    db_path = cfg.get("database", {}).get("path", "jarvis_edge.db")
    if not os.path.isabs(db_path):
        return str(_HERE / db_path)
    return db_path
