import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Clock, StickyNote, AlertTriangle, X, Radio, Mic } from 'lucide-react'
import { api } from '../api.js'

function Waveform({ active }) {
  const bars = Array.from({ length: 24 })
  return (
    <div className="flex items-end justify-center gap-1 h-16">
      {bars.map((_, i) => (
        <span
          key={i}
          className="wave-bar"
          style={{
            height: active ? `${20 + Math.abs(Math.sin(i * 0.7)) * 44}%` : '8%',
            animationDelay: `${i * 0.05}s`,
            animationPlayState: active ? 'running' : 'paused',
            opacity: active ? 1 : 0.25,
          }}
        />
      ))}
    </div>
  )
}

function QuickAction({ icon: Icon, label, color, onClick }) {
  return (
    <motion.button
      whileTap={{ scale: 0.96 }}
      onClick={onClick}
      className="glass glow-border rounded-xl px-4 py-4 flex flex-col items-center gap-2 text-sm text-white"
    >
      <Icon size={20} style={{ color }} />
      {label}
    </motion.button>
  )
}

function TextModal({ title, onCancel, onSubmit }) {
  const [text, setText] = useState('')
  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      className="fixed inset-0 bg-black/60 z-50 flex items-center justify-center p-4"
      onClick={onCancel}
    >
      <motion.div
        initial={{ scale: 0.95, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        exit={{ scale: 0.95, opacity: 0 }}
        onClick={(e) => e.stopPropagation()}
        className="glass rounded-xl p-5 w-full max-w-sm"
      >
        <div className="flex items-center justify-between mb-3">
          <h3 className="text-white font-semibold text-sm">{title}</h3>
          <button onClick={onCancel} className="text-jarvis-muted hover:text-white">
            <X size={16} />
          </button>
        </div>
        <textarea
          autoFocus
          value={text}
          onChange={(e) => setText(e.target.value)}
          rows={3}
          className="w-full bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50 resize-none"
          placeholder="Type here…"
        />
        <div className="flex justify-end gap-2 mt-3">
          <button onClick={onCancel} className="text-xs text-jarvis-muted hover:text-white px-3 py-1.5">
            Cancel
          </button>
          <button
            onClick={() => text.trim() && onSubmit(text.trim())}
            className="text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-3 py-1.5 hover:bg-jarvis-cyan/25"
          >
            Send
          </button>
        </div>
      </motion.div>
    </motion.div>
  )
}

export default function DashboardPage() {
  const [feedText, setFeedText] = useState('Jarvis is ready.')
  const [lastLog, setLastLog] = useState(null)
  const [modal, setModal] = useState(null) // 'note' | 'alert' | null
  const [toast, setToast] = useState('')

  const refresh = useCallback(async () => {
    try {
      const logs = await api.getLogs(1)
      if (logs?.length) {
        setLastLog(logs[0])
        setFeedText(logs[0].fast_response || logs[0].raw_text || 'Jarvis is ready.')
      }
    } catch {
      /* backend unreachable — leave last known feed text on screen */
    }
  }, [])

  useEffect(() => {
    refresh()
    const id = setInterval(refresh, 6000)
    return () => clearInterval(id)
  }, [refresh])

  function flashToast(msg) {
    setToast(msg)
    setTimeout(() => setToast(''), 2500)
  }

  async function fire(type, text = '') {
    try {
      await api.triggerAction(type, text)
      flashToast(`${type.replace('_', ' ')} sent`)
      setModal(null)
      setTimeout(refresh, 800)
    } catch (e) {
      flashToast(`Failed: ${e.message}`)
    }
  }

  return (
    <div className="max-w-4xl mx-auto space-y-6 animate-fade-in">
      <div className="glass rounded-2xl p-8 text-center relative overflow-hidden">
        <div className="flex items-center justify-center gap-2 text-jarvis-cyan text-xs uppercase tracking-widest mb-3">
          <Radio size={13} className="animate-pulse" />
          Jarvis Feed
        </div>
        <motion.p
          key={feedText}
          initial={{ opacity: 0, y: 6 }}
          animate={{ opacity: 1, y: 0 }}
          className="text-2xl md:text-3xl font-medium text-white text-glow leading-snug"
        >
          {feedText}
        </motion.p>
        {lastLog && (
          <p className="text-xs text-jarvis-muted mt-4">
            Last capture &middot; {new Date(lastLog.created_at).toLocaleString()} &middot; status: {lastLog.status}
          </p>
        )}
        <div className="mt-6">
          <Waveform active={!!lastLog && lastLog.status !== 'processed'} />
          <div className="flex items-center justify-center gap-1.5 text-[11px] text-jarvis-muted mt-1">
            <Mic size={11} /> Hold BOOT on the device to capture a thought
          </div>
        </div>
      </div>

      <div>
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted mb-3">Quick Actions</h2>
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
          <QuickAction icon={Clock} label="Time Track" color="#2196F3" onClick={() => fire('time_track')} />
          <QuickAction icon={StickyNote} label="Note" color="#4CAF50" onClick={() => setModal('note')} />
          <QuickAction icon={AlertTriangle} label="Alert" color="#F44336" onClick={() => setModal('alert')} />
          <QuickAction icon={X} label="Dismiss" color="#8A8A99" onClick={() => fire('dismiss')} />
        </div>
      </div>

      <AnimatePresence>
        {modal && (
          <TextModal
            title={modal === 'note' ? 'New Note' : 'New Alert'}
            onCancel={() => setModal(null)}
            onSubmit={(text) => fire(modal, text)}
          />
        )}
      </AnimatePresence>

      <AnimatePresence>
        {toast && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: 20 }}
            className="fixed bottom-6 left-1/2 -translate-x-1/2 glass rounded-full px-4 py-2 text-xs text-white z-50"
          >
            {toast}
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  )
}
