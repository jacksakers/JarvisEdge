#!/usr/bin/env bash
# Launches the backend (FastAPI) and frontend (Vite dev server) together.
# Ctrl+C stops both.
set -euo pipefail
# Job control so each background job gets its own process group — lets
# cleanup() kill -TERM "-$PID" reach child processes too (e.g. npm's vite).
set -m

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BACKEND_PORT=8010
FRONTEND_PORT=5180

# A previous run of this script (or a crashed/orphaned one) can leave its
# backend/frontend still bound to these ports, which makes the new backend
# fail to start with "address already in use" while the frontend looks fine.
kill_port_holder() {
    local port="$1"
    local pids
    pids=$(lsof -ti "tcp:$port" 2>/dev/null || true)
    if [ -n "$pids" ]; then
        echo "==> Port $port already in use (pid(s): $pids) — stopping old process."
        kill $pids 2>/dev/null || true
        sleep 1
        kill -9 $pids 2>/dev/null || true
    fi
}

kill_port_holder "$BACKEND_PORT"
kill_port_holder "$FRONTEND_PORT"

cleanup() {
    echo
    echo "==> Stopping..."
    [ -n "${BACKEND_PID:-}" ] && kill -TERM "-$BACKEND_PID" 2>/dev/null || true
    [ -n "${FRONTEND_PID:-}" ] && kill -TERM "-$FRONTEND_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "==> Starting backend (http://localhost:$BACKEND_PORT)"
cd "$ROOT/backend"
./.venv/bin/python run.py &
BACKEND_PID=$!

echo "==> Starting frontend (http://localhost:$FRONTEND_PORT)"
cd "$ROOT/frontend"
npm run dev -- --port "$FRONTEND_PORT" --strictPort &
FRONTEND_PID=$!

echo
echo "Backend:  http://localhost:$BACKEND_PORT  (docs at /docs)"
echo "Frontend: http://localhost:$FRONTEND_PORT  <- open this in a browser"
echo "Press Ctrl+C to stop both."

wait
