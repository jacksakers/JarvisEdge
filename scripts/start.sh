#!/usr/bin/env bash
# Launches the backend (FastAPI) and frontend (Vite dev server) together.
# Ctrl+C stops both.
set -euo pipefail
# Job control so each background job gets its own process group — lets
# cleanup() kill -TERM "-$PID" reach child processes too (e.g. npm's vite).
set -m

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cleanup() {
    echo
    echo "==> Stopping..."
    [ -n "${BACKEND_PID:-}" ] && kill -TERM "-$BACKEND_PID" 2>/dev/null || true
    [ -n "${FRONTEND_PID:-}" ] && kill -TERM "-$FRONTEND_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "==> Starting backend (http://localhost:8010)"
cd "$ROOT/backend"
./.venv/bin/python run.py &
BACKEND_PID=$!

echo "==> Starting frontend (http://localhost:5180)"
cd "$ROOT/frontend"
npm run dev -- --port 5180 --strictPort &
FRONTEND_PID=$!

echo
echo "Backend:  http://localhost:8010  (docs at /docs)"
echo "Frontend: http://localhost:5180  <- open this in a browser"
echo "Press Ctrl+C to stop both."

wait
