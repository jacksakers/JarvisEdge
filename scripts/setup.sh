#!/usr/bin/env bash
# One-time setup: backend venv + deps, frontend npm deps.
# Run from anywhere; paths are resolved relative to this script.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "==> Backend: creating venv + installing requirements"
cd "$ROOT/backend"
if [ ! -d .venv ]; then
    python3 -m venv .venv
fi
./.venv/bin/pip install -q --upgrade pip
./.venv/bin/pip install -q -r requirements.txt

echo "==> Frontend: installing npm packages"
cd "$ROOT/frontend"
npm install

echo "==> Setup complete."
echo "    Make sure Ollama is running with the models from backend/config.yaml pulled:"
echo "      ollama pull \$(grep fast_model  $ROOT/backend/config.yaml | awk '{print \$2}')"
echo "      ollama pull \$(grep heavy_model $ROOT/backend/config.yaml | awk '{print \$2}')"
echo "    Run '$ROOT/scripts/start.sh' to launch the backend + frontend."
