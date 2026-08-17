import { useEffect, useState } from 'react'
import { api } from '../api.js'

export default function SettingsPage() {
  const [settings, setSettings] = useState(null)
  const [models, setModels] = useState([])
  const [status, setStatus] = useState('')
  const [error, setError] = useState('')

  useEffect(() => {
    load()
  }, [])

  async function load() {
    setError('')
    try {
      const [s, m] = await Promise.all([api.getSettings(), api.getModels()])
      setSettings(s)
      setModels(m.models || [])
    } catch (e) {
      setError(e.message)
    }
  }

  function update(field, value) {
    setSettings((prev) => ({ ...prev, [field]: value }))
  }

  async function save() {
    setStatus('Saving...')
    setError('')
    try {
      const updated = await api.updateSettings({
        fast_model: settings.fast_model,
        heavy_model: settings.heavy_model,
        mqtt_host: settings.mqtt_host,
        mqtt_port: Number(settings.mqtt_port),
      })
      setSettings(updated)
      setStatus('Saved.')
    } catch (e) {
      setError(e.message)
      setStatus('')
    }
  }

  if (error) {
    return (
      <div className="panel">
        <p className="error">Failed to load settings: {error}</p>
        <button onClick={load}>Retry</button>
      </div>
    )
  }

  if (!settings) return <p>Loading settings…</p>

  const modelOptions = (current) => {
    const opts = new Set(models)
    if (current) opts.add(current)
    return Array.from(opts)
  }

  return (
    <div className="panel">
      <h2>Settings</h2>

      <label>
        Fast model (quick confirmations)
        <select
          value={settings.fast_model || ''}
          onChange={(e) => update('fast_model', e.target.value)}
        >
          {modelOptions(settings.fast_model).map((m) => (
            <option key={m} value={m}>
              {m}
            </option>
          ))}
        </select>
      </label>

      <label>
        Heavy model (batch processing)
        <select
          value={settings.heavy_model || ''}
          onChange={(e) => update('heavy_model', e.target.value)}
        >
          {modelOptions(settings.heavy_model).map((m) => (
            <option key={m} value={m}>
              {m}
            </option>
          ))}
        </select>
      </label>

      <label>
        MQTT host
        <input
          type="text"
          value={settings.mqtt_host || ''}
          onChange={(e) => update('mqtt_host', e.target.value)}
        />
      </label>

      <label>
        MQTT port
        <input
          type="number"
          value={settings.mqtt_port || ''}
          onChange={(e) => update('mqtt_port', e.target.value)}
        />
      </label>

      <div className="actions">
        <button onClick={save}>Save</button>
        {status && <span className="status">{status}</span>}
      </div>
    </div>
  )
}
