import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Plus, Trash2, Lightbulb, LightbulbOff, Power, Loader2, WifiOff } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

function ZoneCard({ zone, onToggle, onBrightness, onDelete }) {
  const [brightness, setBrightness] = useState(zone.brightness ?? 100)

  useEffect(() => {
    setBrightness(zone.brightness ?? 100)
  }, [zone.brightness])

  const Icon = zone.on ? Lightbulb : LightbulbOff

  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, x: -20 }}
      className="glass glow-border rounded-xl p-4 space-y-3"
    >
      <div className="flex items-start justify-between gap-2">
        <div className="min-w-0">
          <div className="text-sm text-white font-medium truncate">{zone.name}</div>
          {zone.room && <div className="text-[11px] text-jarvis-muted truncate">{zone.room}</div>}
          <div className="text-[10px] text-jarvis-muted/70 font-mono truncate">{zone.ip}</div>
        </div>
        <button
          onClick={() => onDelete(zone.id)}
          className="text-jarvis-muted hover:text-jarvis-red shrink-0"
        >
          <Trash2 size={14} />
        </button>
      </div>

      {!zone.reachable && (
        <div className="flex items-center gap-1.5 text-[11px] text-jarvis-amber">
          <WifiOff size={12} /> Unreachable
        </div>
      )}

      <button
        onClick={() => onToggle(zone.id)}
        disabled={!zone.reachable}
        className={clsx(
          'w-full flex items-center justify-center gap-2 rounded-lg px-3 py-2 text-sm border transition-colors disabled:opacity-40',
          zone.on
            ? 'bg-jarvis-cyan/15 border-jarvis-cyan/40 text-jarvis-cyan-bright'
            : 'bg-jarvis-bg border-jarvis-border text-jarvis-muted hover:text-white',
        )}
      >
        <Icon size={16} />
        {zone.on ? 'On' : 'Off'}
      </button>

      <div className="flex items-center gap-2">
        <input
          type="range"
          min={1}
          max={100}
          value={brightness}
          disabled={!zone.reachable}
          onChange={(e) => setBrightness(Number(e.target.value))}
          onMouseUp={() => onBrightness(zone.id, brightness)}
          onTouchEnd={() => onBrightness(zone.id, brightness)}
          className="flex-1 accent-cyan-500 disabled:opacity-40"
        />
        <span className="text-[11px] text-jarvis-muted w-9 text-right">{brightness}%</span>
      </div>
    </motion.div>
  )
}

export default function TapoPage() {
  const [zones, setZones] = useState([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState('')
  const [form, setForm] = useState({ name: '', room: '', ip: '' })
  const [adding, setAdding] = useState(false)

  const load = useCallback(async () => {
    try {
      setZones(await api.getTapoZones())
      setError('')
    } catch (e) {
      setError(e.message)
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    load()
    const id = setInterval(load, 10000)
    return () => clearInterval(id)
  }, [load])

  async function addZone() {
    if (!form.name.trim() || !form.ip.trim()) return
    setAdding(true)
    try {
      const created = await api.createTapoZone(form)
      setZones((prev) => [...prev, created])
      setForm({ name: '', room: '', ip: '' })
    } catch (e) {
      setError(e.message)
    } finally {
      setAdding(false)
    }
  }

  async function toggleZone(id) {
    setZones((prev) => prev.map((z) => (z.id === id ? { ...z, on: !z.on } : z)))
    try {
      const updated = await api.toggleTapoZone(id)
      setZones((prev) => prev.map((z) => (z.id === id ? updated : z)))
    } catch (e) {
      setError(e.message)
      load()
    }
  }

  async function setBrightness(id, brightness) {
    try {
      const updated = await api.setTapoBrightness(id, brightness)
      setZones((prev) => prev.map((z) => (z.id === id ? updated : z)))
    } catch (e) {
      setError(e.message)
    }
  }

  async function deleteZone(id) {
    setZones((prev) => prev.filter((z) => z.id !== id))
    await api.deleteTapoZone(id)
  }

  async function allOff() {
    setZones((prev) => prev.map((z) => ({ ...z, on: false })))
    await api.tapoAllOff()
    load()
  }

  return (
    <div className="max-w-3xl mx-auto space-y-6 animate-fade-in">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-lg font-semibold text-white">Ambient Home</h1>
          <p className="text-xs text-jarvis-muted mt-1">
            Tapo bulb zones shown as the device's Ambient Home grid. Requires a Tapo account
            email/password set in Settings.
          </p>
        </div>
        <button
          onClick={allOff}
          className="text-xs bg-jarvis-red/10 border border-jarvis-red/30 text-jarvis-red rounded-lg px-3 py-1.5 hover:bg-jarvis-red/20 flex items-center gap-1.5 shrink-0"
        >
          <Power size={13} /> All Off
        </button>
      </div>

      <div className="glass rounded-2xl p-4 space-y-3">
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted">Add a zone</h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-2">
          <input
            value={form.name}
            onChange={(e) => setForm((f) => ({ ...f, name: e.target.value }))}
            placeholder="Name (e.g. Living Room)"
            className="bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50"
          />
          <input
            value={form.room}
            onChange={(e) => setForm((f) => ({ ...f, room: e.target.value }))}
            placeholder="Room (optional)"
            className="bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50"
          />
          <input
            value={form.ip}
            onChange={(e) => setForm((f) => ({ ...f, ip: e.target.value }))}
            placeholder="Bulb IP (e.g. 192.168.1.42)"
            className="bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50"
          />
        </div>
        <button
          onClick={addZone}
          disabled={adding}
          className="text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-3 py-1.5 hover:bg-jarvis-cyan/25 flex items-center gap-1.5 disabled:opacity-50"
        >
          <Plus size={14} /> Add Zone
        </button>
      </div>

      {error && <p className="text-xs text-jarvis-red">{error}</p>}
      {loading && (
        <div className="flex justify-center py-8 text-jarvis-muted">
          <Loader2 size={20} className="animate-spin" />
        </div>
      )}
      {!loading && zones.length === 0 && (
        <p className="text-sm text-jarvis-muted text-center py-8">No zones configured yet — add one above.</p>
      )}

      <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-3">
        <AnimatePresence initial={false}>
          {zones.map((zone) => (
            <ZoneCard
              key={zone.id}
              zone={zone}
              onToggle={toggleZone}
              onBrightness={setBrightness}
              onDelete={deleteZone}
            />
          ))}
        </AnimatePresence>
      </div>
    </div>
  )
}
