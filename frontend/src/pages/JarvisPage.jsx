import { useEffect, useState, useCallback } from 'react'
import { motion } from 'framer-motion'
import { Cpu, CheckCircle2, XCircle, RefreshCw } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

export default function JarvisPage() {
  const [status, setStatus] = useState(null)
  const [feed, setFeed] = useState([])
  const [checking, setChecking] = useState(false)
  const [error, setError] = useState('')

  const load = useCallback(async () => {
    setChecking(true)
    setError('')
    try {
      const s = await api.getJarvisStatus()
      setStatus(s)
      if (s?.enabled && s?.connected) {
        setFeed(await api.getJarvisFeed(20))
      }
    } catch (e) {
      setError(e.message)
    } finally {
      setChecking(false)
    }
  }, [])

  useEffect(() => {
    load()
    const id = setInterval(load, 10000)
    return () => clearInterval(id)
  }, [load])

  const enabled = !!status?.enabled
  const connected = !!status?.connected

  return (
    <div className="max-w-2xl mx-auto space-y-6 animate-fade-in">
      <div>
        <h1 className="text-lg font-semibold text-white">JARVIS 3.0 Link</h1>
        <p className="text-xs text-jarvis-muted mt-1">
          This Edge Node can optionally delegate heavy tasks and journal notes to a full JARVIS 3.0 instance.
        </p>
      </div>

      <div className="glass rounded-2xl p-6 flex items-center gap-4">
        <div
          className={clsx(
            'w-14 h-14 rounded-xl border flex items-center justify-center shrink-0',
            connected ? 'border-jarvis-green/40 bg-jarvis-green/10' : 'border-jarvis-red/40 bg-jarvis-red/10',
          )}
        >
          <Cpu size={24} className={connected ? 'text-jarvis-green' : 'text-jarvis-red'} />
        </div>
        <div className="flex-1">
          <div className="text-white font-medium flex items-center gap-2">
            {!enabled ? 'Integration disabled' : connected ? 'Connected' : 'Unreachable'}
            {enabled && (connected ? <CheckCircle2 size={15} className="text-jarvis-green" /> : <XCircle size={15} className="text-jarvis-red" />)}
          </div>
          <div className="text-xs text-jarvis-muted mt-1">
            {enabled
              ? 'Enable/disable and base URL are configured on the Settings page.'
              : 'Turn this on from Settings to link with your JARVIS 3.0 framework.'}
          </div>
        </div>
        <button
          onClick={load}
          className="text-jarvis-muted hover:text-white p-2 rounded-lg hover:bg-jarvis-surface shrink-0"
        >
          <RefreshCw size={16} className={checking ? 'animate-spin' : ''} />
        </button>
      </div>

      {error && <p className="text-xs text-jarvis-red">{error}</p>}

      {enabled && connected && (
        <div>
          <h2 className="text-xs uppercase tracking-widest text-jarvis-muted mb-3">JARVIS Feed (read-only)</h2>
          {feed.length === 0 && <p className="text-sm text-jarvis-muted">No recent JARVIS feed items.</p>}
          <div className="space-y-2">
            {feed.map((item, i) => (
              <motion.div
                key={item.id ?? i}
                initial={{ opacity: 0, y: 6 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: i * 0.02 }}
                className="glass rounded-xl px-4 py-3 text-sm text-jarvis-text"
              >
                {item.text || item.title || JSON.stringify(item)}
              </motion.div>
            ))}
          </div>
        </div>
      )}
    </div>
  )
}
