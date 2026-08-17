import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Clock, StickyNote, AlertTriangle, X, Trash2, CheckCircle2 } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

const TYPE_META = {
  time_track: { icon: Clock, color: '#2196F3', label: 'Time Track' },
  note: { icon: StickyNote, color: '#4CAF50', label: 'Note' },
  alert: { icon: AlertTriangle, color: '#F44336', label: 'Alert' },
  dismiss: { icon: X, color: '#8A8A99', label: 'Dismiss' },
}

function TextPrompt({ label, onCancel, onSubmit }) {
  const [text, setText] = useState('')
  return (
    <motion.div
      initial={{ opacity: 0, height: 0 }}
      animate={{ opacity: 1, height: 'auto' }}
      exit={{ opacity: 0, height: 0 }}
      className="overflow-hidden"
    >
      <div className="flex gap-2 mt-3">
        <input
          autoFocus
          value={text}
          onChange={(e) => setText(e.target.value)}
          onKeyDown={(e) => e.key === 'Enter' && text.trim() && onSubmit(text.trim())}
          placeholder={`${label}…`}
          className="flex-1 bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-1.5 text-sm text-white outline-none focus:border-jarvis-cyan/50"
        />
        <button onClick={onCancel} className="text-xs text-jarvis-muted hover:text-white px-2">
          Cancel
        </button>
        <button
          onClick={() => text.trim() && onSubmit(text.trim())}
          className="text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-3 hover:bg-jarvis-cyan/25"
        >
          Send
        </button>
      </div>
    </motion.div>
  )
}

export default function ActionsPage() {
  const [events, setEvents] = useState([])
  const [loading, setLoading] = useState(true)
  const [activePrompt, setActivePrompt] = useState(null)

  const load = useCallback(async () => {
    try {
      setEvents(await api.getActions(50))
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    load()
    const id = setInterval(load, 8000)
    return () => clearInterval(id)
  }, [load])

  async function fire(type, text = '') {
    setActivePrompt(null)
    await api.triggerAction(type, text)
    load()
  }

  async function remove(id) {
    setEvents((prev) => prev.filter((e) => e.id !== id))
    await api.deleteAction(id)
  }

  async function clearAll() {
    setEvents([])
    await api.clearActions()
  }

  return (
    <div className="max-w-2xl mx-auto space-y-6 animate-fade-in">
      <div>
        <h1 className="text-lg font-semibold text-white">Action Grid</h1>
        <p className="text-xs text-jarvis-muted mt-1">
          Mirrors the device's 2×2 Action Grid tile — trigger from here, or watch what the device sends.
        </p>
      </div>

      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        {Object.entries(TYPE_META).map(([type, meta]) => {
          const Icon = meta.icon
          const needsText = type === 'note' || type === 'alert'
          return (
            <button
              key={type}
              onClick={() => (needsText ? setActivePrompt(type) : fire(type))}
              className="glass glow-border rounded-xl px-4 py-4 flex flex-col items-center gap-2 text-sm text-white"
            >
              <Icon size={20} style={{ color: meta.color }} />
              {meta.label}
            </button>
          )
        })}
      </div>

      <AnimatePresence>
        {activePrompt && (
          <TextPrompt
            label={TYPE_META[activePrompt].label}
            onCancel={() => setActivePrompt(null)}
            onSubmit={(text) => fire(activePrompt, text)}
          />
        )}
      </AnimatePresence>

      <div className="flex items-center justify-between pt-2">
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted">History</h2>
        {events.length > 0 && (
          <button onClick={clearAll} className="text-xs text-jarvis-muted hover:text-jarvis-red">
            Clear all
          </button>
        )}
      </div>

      {loading && <p className="text-sm text-jarvis-muted text-center py-6">Loading…</p>}
      {!loading && events.length === 0 && (
        <p className="text-sm text-jarvis-muted text-center py-6">No actions triggered yet.</p>
      )}

      <div className="space-y-2">
        <AnimatePresence initial={false}>
          {events.map((ev) => {
            const meta = TYPE_META[ev.action_type] || TYPE_META.dismiss
            const Icon = meta.icon
            return (
              <motion.div
                key={ev.id}
                layout
                initial={{ opacity: 0, y: 8 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0, x: -20 }}
                className="group flex items-center gap-3 glass rounded-xl px-4 py-3"
              >
                <Icon size={16} style={{ color: meta.color }} className="shrink-0" />
                <div className="flex-1 min-w-0">
                  <div className="text-sm text-white">{meta.label}</div>
                  {ev.text && <div className="text-xs text-jarvis-muted truncate">{ev.text}</div>}
                </div>
                {ev.jarvis_synced && (
                  <span
                    title="Synced to JARVIS 3.0"
                    className="flex items-center gap-1 text-[10px] text-jarvis-purple shrink-0"
                  >
                    <CheckCircle2 size={12} /> JARVIS
                  </span>
                )}
                <span className="text-[11px] text-jarvis-muted shrink-0">
                  {new Date(ev.created_at).toLocaleTimeString()}
                </span>
                <button
                  onClick={() => remove(ev.id)}
                  className={clsx(
                    'text-jarvis-muted hover:text-jarvis-red opacity-0 group-hover:opacity-100 transition-opacity shrink-0',
                  )}
                >
                  <Trash2 size={14} />
                </button>
              </motion.div>
            )
          })}
        </AnimatePresence>
      </div>
    </div>
  )
}
