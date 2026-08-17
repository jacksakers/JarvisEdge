import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { RefreshCw, Loader2, Plus } from 'lucide-react'
import { api } from '../api.js'
import LogRow from '../components/LogRow.jsx'

export default function LogsPage() {
  const [logs, setLogs] = useState(null)
  const [limit, setLimit] = useState(20)
  const [error, setError] = useState('')
  const [showCreate, setShowCreate] = useState(false)
  const [newText, setNewText] = useState('')
  const [newResponse, setNewResponse] = useState('')

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

  async function update(id, fields) {
    const updated = await api.updateLog(id, fields)
    setLogs((prev) => prev.map((l) => (l.id === id ? { ...l, ...updated } : l)))
  }

  async function clearAll() {
    if (!window.confirm('Delete all voice logs?')) return
    setLogs([])
    await api.clearLogs()
  }

  async function handleCreate(e) {
    e.preventDefault()
    if (!newText.trim()) return
    try {
      const created = await api.createLog({ raw_text: newText, fast_response: newResponse })
      setLogs((prev) => [created, ...(prev || [])])
      setNewText('')
      setNewResponse('')
      setShowCreate(false)
    } catch (err) {
      setError('Failed to create log: ' + err.message)
    }
  }

  return (
    <div className="max-w-3xl mx-auto space-y-6 animate-fade-in">
      <div className="flex items-center justify-between flex-wrap gap-3">
        <div>
          <h1 className="text-lg font-semibold text-white">Voice Logs</h1>
          <p className="text-xs text-jarvis-muted mt-1">
            Raw audio is kept for playback/debugging (disable via backend config.yaml `audio.keep_files`).
          </p>
        </div>
        <div className="flex items-center gap-2">
          <button
            onClick={() => setShowCreate(!showCreate)}
            className="flex items-center gap-1 text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright hover:bg-jarvis-cyan/25 px-2.5 py-1.5 rounded-lg font-medium"
          >
            <Plus size={14} /> New Log
          </button>
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

      <AnimatePresence>
        {showCreate && (
          <motion.form
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            onSubmit={handleCreate}
            className="glass rounded-xl p-4 space-y-3 overflow-hidden border border-jarvis-cyan/20"
          >
            <h3 className="text-xs font-semibold uppercase tracking-wider text-jarvis-cyan">Manually Add Voice Log Entry</h3>
            <div>
              <label className="text-jarvis-muted uppercase tracking-widest text-[9px] block mb-1">Transcript Text</label>
              <textarea
                required
                value={newText}
                onChange={(e) => setNewText(e.target.value)}
                placeholder="Type manual transcript conversation..."
                className="w-full bg-jarvis-bg border border-jarvis-border rounded-lg p-2 text-white outline-none text-xs focus:border-jarvis-cyan"
                rows={3}
              />
            </div>
            <div>
              <label className="text-jarvis-muted uppercase tracking-widest text-[9px] block mb-1">Fast Response (optional)</label>
              <input
                type="text"
                value={newResponse}
                onChange={(e) => setNewResponse(e.target.value)}
                placeholder="Immediate AI confirmation reply..."
                className="w-full bg-jarvis-bg border border-jarvis-border rounded-lg p-2 text-white outline-none text-xs focus:border-jarvis-cyan"
              />
            </div>
            <div className="flex justify-end gap-2 pt-1">
              <button
                type="button"
                onClick={() => setShowCreate(false)}
                className="px-3 py-1.5 bg-jarvis-surface border border-jarvis-border text-xs rounded text-jarvis-muted hover:text-white"
              >
                Cancel
              </button>
              <button
                type="submit"
                className="px-3 py-1.5 bg-jarvis-cyan text-black font-semibold text-xs rounded hover:bg-jarvis-cyan-bright"
              >
                Add Log
              </button>
            </div>
          </motion.form>
        )}
      </AnimatePresence>

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
            <LogRow key={log.id} log={log} onDelete={remove} onUpdate={update} />
          ))}
        </AnimatePresence>
      </div>
    </div>
  )
}