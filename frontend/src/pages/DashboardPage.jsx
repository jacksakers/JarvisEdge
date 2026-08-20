import { useEffect, useState, useCallback } from 'react'
import { motion } from 'framer-motion'
import { Link } from 'react-router-dom'
import { Radio, Mic, ListTodo, Lightbulb, ScrollText } from 'lucide-react'
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

function QuickLink({ to, icon: Icon, label, color }) {
  return (
    <Link
      to={to}
      className="glass glow-border rounded-xl px-4 py-4 flex flex-col items-center gap-2 text-sm text-white"
    >
      <Icon size={20} style={{ color }} />
      {label}
    </Link>
  )
}

export default function DashboardPage() {
  const [feedText, setFeedText] = useState('Jarvis is ready.')
  const [lastLog, setLastLog] = useState(null)

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

  return (
    <div className="max-w-4xl mx-auto space-y-6 animate-fade-in">
      <div className="glass rounded-2xl p-8 text-center relative overflow-hidden">
        <div className="flex items-center justify-center gap-2 text-jarvis-cyan text-xs uppercase tracking-widest mb-3">
          <Radio size={13} className="animate-pulse" />
          Jarvis Voice Capture
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
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted mb-3">Quick Links</h2>
        <div className="grid grid-cols-2 md:grid-cols-3 gap-3">
          <QuickLink to="/tapo" icon={Lightbulb} label="Ambient Home" color="#FFC107" />
          <QuickLink to="/todo" icon={ListTodo} label="Todo List" color="#4CAF50" />
          <QuickLink to="/logs" icon={ScrollText} label="Voice Logs" color="#2196F3" />
        </div>
      </div>
    </div>
  )
}
