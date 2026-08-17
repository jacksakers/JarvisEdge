import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { ChevronDown, Trash2, RefreshCw, Loader2 } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

const STATUS_META = {
  pending: { label: 'Pending', color: 'text-jarvis-amber border-jarvis-amber/30 bg-jarvis-amber/10' },
  fast_done: { label: 'Fast done', color: 'text-jarvis-cyan border-jarvis-cyan/30 bg-jarvis-cyan/10' },
  processed: { label: 'Processed', color: 'text-jarvis-green border-jarvis-green/30 bg-jarvis-green/10' },
  failed: { label: 'Failed', color: 'text-jarvis-red border-jarvis-red/30 bg-jarvis-red/10' },
}

function LogRow({ log, onDelete }) {
  const [open, setOpen] = useState(false)
  const meta = STATUS_META[log.status] || STATUS_META.pending

  return (
    <motion.div layout initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="glass rounded-xl overflow-hidden">
      <button
        onClick={() => setOpen((v) => !v)}
        className="w-full flex items-center gap-3 px-4 py-3 text-left"
      >
        <ChevronDown size={14} className={clsx('text-jarvis-muted transition-transform shrink-0', open && 'rotate-180')} />
        <div className="flex-1 min-w-0">
          <div className="text-sm text-white truncate">{log.raw_text || <em className="text-jarvis-muted">(no transcript)</em>}</div>
          <div className="text-[11px] text-jarvis-muted">{new Date(log.created_at).toLocaleString()}</div>
        </div>
        <span className={clsx('text-[10px] px-2 py-0.5 rounded-full border shrink-0', meta.color)}>{meta.label}</span>
        <button
          onClick={(e) => {
            e.stopPropagation()
            onDelete(log.id)
          }}
          className="text-jarvis-muted hover:text-jarvis-red shrink-0"
        >
          <Trash2 size={14} />
        </button>
      </button>

      <AnimatePresence>
        {open && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            className="overflow-hidden border-t border-jarvis-border"
          >
            <div className="px-4 py-3 space-y-3 text-xs">
              <div>
                <div className="text-jarvis-muted uppercase tracking-widest text-[10px] mb-1">Fast response</div>
                <div className="text-jarvis-text">{log.fast_response || '—'}</div>
              </div>
              {log.structured_data && (
                <div>
                  <div className="text-jarvis-muted uppercase tracking-widest text-[10px] mb-1">Structured data</div>
                  <pre className="bg-jarvis-bg border border-jarvis-border rounded-lg p-3 overflow-x-auto text-jarvis-cyan-bright font-mono text-[11px]">
                    {JSON.stringify(log.structured_data, null, 2)}
                  </pre>
                </div>
              )}
              {log.jarvis_task_id && (
                <div className="text-jarvis-purple">JARVIS task: {log.jarvis_task_id}</div>
              )}
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  )
}

export default function LogsPage() {
  const [logs, setLogs] = useState(null)
  const [limit, setLimit] = useState(20)
  const [error, setError] = useState('')

  const load = useCallback(async () => {
    setError('')
    try {
      setLogs(await api.getLogs(limit))
    } catch (e) {
      setError(e.message)
    }
  }, [limit])

  useEffect(() => {
    load()
    const id = setInterval(load, 8000)
    return () => clearInterval(id)
  }, [load])

  async function remove(id) {
    setLogs((prev) => prev.filter((l) => l.id !== id))
    await api.deleteLog(id)
  }

  async function clearAll() {
    if (!window.confirm('Delete all voice logs?')) return
    setLogs([])
    await api.clearLogs()
  }

  return (
    <div className="max-w-3xl mx-auto space-y-6 animate-fade-in">
      <div className="flex items-center justify-between flex-wrap gap-3">
        <div>
          <h1 className="text-lg font-semibold text-white">Voice Logs</h1>
          <p className="text-xs text-jarvis-muted mt-1">
            Transcripts are kept; raw audio is discarded after transcription for privacy.
          </p>
        </div>
        <div className="flex items-center gap-2">
          <select
            value={limit}
            onChange={(e) => setLimit(Number(e.target.value))}
            className="bg-jarvis-surface border border-jarvis-border rounded-lg px-2 py-1.5 text-xs text-white outline-none"
          >
            <option value={20}>Last 20</option>
            <option value={50}>Last 50</option>
            <option value={100}>Last 100</option>
          </select>
          <button onClick={load} className="text-jarvis-muted hover:text-white p-1.5 rounded-lg hover:bg-jarvis-surface">
            <RefreshCw size={14} />
          </button>
          {logs?.length > 0 && (
            <button onClick={clearAll} className="text-xs text-jarvis-muted hover:text-jarvis-red">
              Clear all
            </button>
          )}
        </div>
      </div>

      {error && <p className="text-xs text-jarvis-red">{error}</p>}
      {!logs && (
        <div className="flex justify-center py-10 text-jarvis-muted">
          <Loader2 size={20} className="animate-spin" />
        </div>
      )}
      {logs?.length === 0 && <p className="text-sm text-jarvis-muted text-center py-10">No entries yet.</p>}

      <div className="space-y-2">
        <AnimatePresence initial={false}>
          {logs?.map((log) => (
            <LogRow key={log.id} log={log} onDelete={remove} />
          ))}
        </AnimatePresence>
      </div>
    </div>
  )
}
