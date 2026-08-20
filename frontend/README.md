# House Phone — Command Center (frontend)

The web dashboard for a House Phone device. It talks to the FastAPI [backend](../backend/README.md)
over plain HTTP and gives you full control from a browser: live voice-capture feed, Ambient Home
(Tapo zone) CRUD + control, the Todo List, voice log history, and JARVIS 3.0 integration status/settings.

Built with React 19 + Vite, Tailwind CSS v4, `react-router-dom`, `framer-motion` and `lucide-react`,
using the same dark "jarvis-*" design tokens (cyan glass panels, subtle grid background, glow
accents) as the JARVIS 3.0 Command Center, for a consistent look across the whole framework.

## Pages

| Route       | Purpose                                                                 |
|------------|---------------------------------------------------------------------------|
| `/`         | Jarvis Voice Capture — shows the latest transcript/response, a decorative activity waveform, and quick links to the other pages. |
| `/tapo`     | Ambient Home — full CRUD for Tapo bulb zones (name/room/IP), live toggle + brightness slider per zone, and an "All Off" button. Mirrors the device's Ambient Home tile. |
| `/todo`     | Todo List — full CRUD (add, edit inline, toggle done, delete). Command-Center-only; there's no on-device tile, but the heavy LLM tier still auto-adds tasks it extracts from voice notes. |
| `/logs`     | Voice Logs — browse transcripts, fast responses, structured data and JARVIS task links. Raw audio is never stored server-side. |
| `/jarvis`   | JARVIS Link — connection status to a full JARVIS 3.0 instance and a read-only mirror of its feed. |
| `/prompts`  | Edit the fast-tier and heavy-tier system prompts used by the backend LLM calls. |
| `/settings` | Ollama model selection, MQTT broker config, Tapo account login, JARVIS 3.0 enable/base URL + test connection, and Pocket Recorder/power settings. |

## Configuring the backend URL

The backend host is not baked in at build time — it's stored in `localStorage`
(`jarvis_backend_url`) so the same static build can point at any House Phone device on your
network. Click the URL pill in the top-right of the header to change it (defaults to
`http://localhost:8010`).

## Development

```bash
npm install
npm run dev      # http://localhost:5180
npm run build    # production build to dist/
```

