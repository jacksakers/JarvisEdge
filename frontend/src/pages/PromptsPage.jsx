import { useEffect, useState } from 'react'
import { Loader2 } from 'lucide-react'
import { api } from '../api.js'

const textareaClass =
  'w-full bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2.5 text-sm text-white outline-none focus:border-jarvis-cyan/50 font-mono resize-y'

export default function PromptsPage() {
  const [prompts, setPrompts] = useState(null)
  const [status, setStatus] = useState('')
  const [error, setError] = useState('')

  useEffect(() => {
    load()
  }, [])

  async function load() {
    setError('')
    try {
      setPrompts(await api.getPrompts())
    } catch (e) {
      setError(e.message)
    }
  }

  function update(field, value) {
    setPrompts((prev) => ({ ...prev, [field]: value }))
  }

  async function save() {
    setStatus('Saving…')
    setError('')
    try {
      const updated = await api.updatePrompts(prompts)
      setPrompts(updated)
      setStatus('Saved.')
      setTimeout(() => setStatus(''), 2000)
    } catch (e) {
      setError(e.message)
      setStatus('')
    }
  }

  if (error) {
    return (
      <div className="max-w-2xl mx-auto glass rounded-xl p-6">
        <p className="text-jarvis-red text-sm">Failed to load prompts: {error}</p>
        <button onClick={load} className="mt-3 text-xs text-jarvis-cyan-bright">
          Retry
        </button>
      </div>
    )
  }

  if (!prompts) {
    return (
      <div className="flex justify-center py-16 text-jarvis-muted">
        <Loader2 size={20} className="animate-spin" />
      </div>
    )
  }

  return (
    <div className="max-w-2xl mx-auto space-y-6 animate-fade-in">
      <h1 className="text-lg font-semibold text-white">Prompts</h1>

      <section className="glass rounded-2xl p-5 space-y-2">
        <span className="block text-xs text-jarvis-muted">Fast-tier system prompt</span>
        <textarea
          rows={6}
          value={prompts.fast_system_prompt || ''}
          onChange={(e) => update('fast_system_prompt', e.target.value)}
          className={textareaClass}
        />
      </section>

      <section className="glass rounded-2xl p-5 space-y-2">
        <span className="block text-xs text-jarvis-muted">Heavy-tier system prompt</span>
        <textarea
          rows={12}
          value={prompts.heavy_system_prompt || ''}
          onChange={(e) => update('heavy_system_prompt', e.target.value)}
          className={textareaClass}
        />
      </section>

      <div className="flex items-center gap-3">
        <button
          onClick={save}
          className="bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-5 py-2 text-sm hover:bg-jarvis-cyan/25"
        >
          Save Prompts
        </button>
        {status && <span className="text-xs text-jarvis-muted">{status}</span>}
      </div>
    </div>
  )
}
