# Jarvis Edge — Command Center (frontend)

The web dashboard for a Jarvis Edge node. It talks to the FastAPI [backend](../backend/README.md)
over plain HTTP and gives you full control of the device from a browser: live feed, daily focus
CRUD, the action grid, voice log history, and JARVIS 3.0 integration status/settings.

Built with React 19 + Vite, Tailwind CSS v4, `react-router-dom`, `framer-motion` and `lucide-react`,
using the same dark "jarvis-*" design tokens (cyan glass panels, subtle grid background, glow
accents) as the JARVIS 3.0 Command Center, for a consistent look across the whole framework.

## Pages

| Route       | Purpose                                                                 |
|------------|---------------------------------------------------------------------------|
| `/`         | Jarvis Feed — shows the latest transcript/response, a decorative activity waveform, and one-tap Quick Actions (Time Track / Note / Alert / Dismiss). |
| `/focus`    | Daily Focus — full CRUD (add, edit inline, toggle done, delete) for the items pushed to the device's Daily Focus tile over MQTT. |
| `/actions`  | Action Grid — trigger actions from the browser and view/delete the action history. |
| `/logs`     | Voice Logs — browse transcripts, fast responses, structured data and JARVIS task links. Raw audio is never stored server-side. |
| `/jarvis`   | JARVIS Link — connection status to a full JARVIS 3.0 instance and a read-only mirror of its feed. |
| `/prompts`  | Edit the fast-tier and heavy-tier system prompts used by the backend LLM calls. |
| `/settings` | Ollama model selection, MQTT broker config, and JARVIS 3.0 enable/base URL + test connection. |

## Configuring the backend URL

The backend host is not baked in at build time — it's stored in `localStorage`
(`jarvis_backend_url`) so the same static build can point at any Edge Node on your network.
Click the URL pill in the top-right of the header to change it (defaults to
`http://localhost:8010`).

## Development

```bash
npm install
npm run dev      # http://localhost:5180
npm run build    # production build to dist/
```

