import { useEffect, useState } from 'react'
import { api } from '../api.js'

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
    setStatus('Saving...')
    setError('')
    try {
      const updated = await api.updatePrompts(prompts)
      setPrompts(updated)
      setStatus('Saved.')
    } catch (e) {
      setError(e.message)
      setStatus('')
    }
  }

  if (error) {
    return (
      <div className="panel">
        <p className="error">Failed to load prompts: {error}</p>
        <button onClick={load}>Retry</button>
      </div>
    )
  }

  if (!prompts) return <p>Loading prompts…</p>

  return (
    <div className="panel">
      <h2>Prompts</h2>

      <label>
        Fast-tier system prompt
        <textarea
          rows={6}
          value={prompts.fast_system_prompt || ''}
          onChange={(e) => update('fast_system_prompt', e.target.value)}
        />
      </label>

      <label>
        Heavy-tier system prompt
        <textarea
          rows={10}
          value={prompts.heavy_system_prompt || ''}
          onChange={(e) => update('heavy_system_prompt', e.target.value)}
        />
      </label>

      <div className="actions">
        <button onClick={save}>Save</button>
        {status && <span className="status">{status}</span>}
      </div>
    </div>
  )
}
