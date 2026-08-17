import { useEffect, useState } from 'react'
import { api } from '../api.js'

// Polls GET /health every `intervalMs` and exposes connection state.
// Used by AppShell to drive the sidebar's live status dot.
export default function useHealth(intervalMs = 8000) {
  const [connected, setConnected] = useState(false)

  useEffect(() => {
    let cancelled = false

    async function check() {
      try {
        await api.getHealth()
        if (!cancelled) setConnected(true)
      } catch {
        if (!cancelled) setConnected(false)
      }
    }

    check()
    const id = setInterval(check, intervalMs)
    return () => {
      cancelled = true
      clearInterval(id)
    }
  }, [intervalMs])

  return connected
}
