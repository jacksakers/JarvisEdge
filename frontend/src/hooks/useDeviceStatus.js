import { useEffect, useState } from 'react'
import { api } from '../api.js'

// Polls GET /device/status every `intervalMs` — reflects the ESP32's own
// heartbeat (device_heartbeat.cpp), distinct from backend reachability.
export default function useDeviceStatus(intervalMs = 10000) {
  const [status, setStatus] = useState({ online: false, last_seen: null })

  useEffect(() => {
    let cancelled = false

    async function check() {
      try {
        const s = await api.getDeviceStatus()
        if (!cancelled) setStatus(s)
      } catch {
        if (!cancelled) setStatus({ online: false, last_seen: null })
      }
    }

    check()
    const id = setInterval(check, intervalMs)
    return () => {
      cancelled = true
      clearInterval(id)
    }
  }, [intervalMs])

  return status
}
