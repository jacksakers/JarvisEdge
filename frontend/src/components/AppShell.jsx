import { useState } from 'react'
import { Outlet } from 'react-router-dom'
import { Settings2, Wifi, WifiOff, Radio, RadioOff } from 'lucide-react'
import Sidebar from './Sidebar.jsx'
import useHealth from '../hooks/useHealth.js'
import useDeviceStatus from '../hooks/useDeviceStatus.js'
import { getBaseUrl, setBaseUrl } from '../api.js'

function BackendUrlPopover({ onClose }) {
  const [url, setUrl] = useState(getBaseUrl())

  function apply() {
    setBaseUrl(url)
    window.location.reload()
  }

  return (
    <div className="absolute right-0 top-full mt-2 w-80 glass rounded-xl p-4 z-50 animate-slide-up">
      <div className="text-xs text-jarvis-muted mb-2">Backend URL</div>
      <div className="flex gap-2">
        <input
          value={url}
          onChange={(e) => setUrl(e.target.value)}
          className="flex-1 bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-1.5 text-sm text-white outline-none focus:border-jarvis-cyan/50"
          placeholder="http://192.168.1.88:8010"
        />
      </div>
      <div className="flex justify-end gap-2 mt-3">
        <button onClick={onClose} className="text-xs text-jarvis-muted hover:text-white px-2 py-1">
          Cancel
        </button>
        <button
          onClick={apply}
          className="text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-3 py-1.5 hover:bg-jarvis-cyan/25"
        >
          Apply &amp; Reload
        </button>
      </div>
    </div>
  )
}

export default function AppShell() {
  const connected = useHealth()
  const device = useDeviceStatus()
  const [showUrlPopover, setShowUrlPopover] = useState(false)

  return (
    <div className="flex h-full bg-jarvis-bg bg-grid">
      <div className="fixed inset-0 bg-hex-glow pointer-events-none" />

      <aside className="hidden md:flex flex-col w-60 shrink-0 glass border-r border-jarvis-border relative z-10">
        <Sidebar connected={connected} />
      </aside>

      <div className="flex flex-col flex-1 min-w-0 relative z-10">
        <header className="flex items-center justify-between px-6 py-3 border-b border-jarvis-border glass md:bg-transparent md:backdrop-blur-none md:border-b md:border-jarvis-border">
          <div className="flex items-center gap-4 text-sm text-jarvis-muted">
            <div className="flex items-center gap-2">
              {connected ? (
                <Wifi size={14} className="text-jarvis-green" />
              ) : (
                <WifiOff size={14} className="text-jarvis-red" />
              )}
              <span className={connected ? 'text-jarvis-green' : 'text-jarvis-red'}>
                {connected ? 'Backend connected' : 'Backend unreachable'}
              </span>
            </div>
            <div className="flex items-center gap-2" title={device.last_seen ? `Last heartbeat: ${new Date(device.last_seen).toLocaleString()}` : 'No heartbeat received yet'}>
              {device.online ? (
                <Radio size={14} className="text-jarvis-green" />
              ) : (
                <RadioOff size={14} className="text-jarvis-red" />
              )}
              <span className={device.online ? 'text-jarvis-green' : 'text-jarvis-red'}>
                {device.online ? 'Device online' : 'Device offline'}
              </span>
            </div>
          </div>

          <div className="relative">
            <button
              onClick={() => setShowUrlPopover((v) => !v)}
              className="flex items-center gap-1.5 text-xs text-jarvis-muted hover:text-white px-2.5 py-1.5 rounded-lg hover:bg-jarvis-surface transition-colors"
            >
              <Settings2 size={13} />
              {getBaseUrl().replace(/^https?:\/\//, '')}
            </button>
            {showUrlPopover && <BackendUrlPopover onClose={() => setShowUrlPopover(false)} />}
          </div>
        </header>

        <main className="flex-1 overflow-y-auto p-4 md:p-6">
          <Outlet />
        </main>
      </div>
    </div>
  )
}
