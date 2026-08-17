import { useEffect, useState } from 'react'
import { CheckCircle2, XCircle, Loader2 } from 'lucide-react'
import { api } from '../api.js'

function Field({ label, children }) {
  return (
    <label className="block">
      <span className="block text-xs text-jarvis-muted mb-1.5">{label}</span>
      {children}
    </label>
  )
}

const inputClass =
  'w-full bg-jarvis-bg border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50'

export default function SettingsPage() {
  const [settings, setSettings] = useState(null)
  const [models, setModels] = useState([])
  const [status, setStatus] = useState('')
  const [error, setError] = useState('')
  const [testResult, setTestResult] = useState(null)
  const [testing, setTesting] = useState(false)

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
    setStatus('Saving…')
    setError('')
    try {
      const updated = await api.updateSettings({
        fast_model: settings.fast_model,
        heavy_model: settings.heavy_model,
        mqtt_host: settings.mqtt_host,
        mqtt_port: Number(settings.mqtt_port),
        jarvis_enabled: !!settings.jarvis_enabled,
        jarvis_base_url: settings.jarvis_base_url,
      })
      setSettings(updated)
      setStatus('Saved.')
      setTimeout(() => setStatus(''), 2000)
    } catch (e) {
      setError(e.message)
      setStatus('')
    }
  }

  async function testConnection() {
    setTesting(true)
    setTestResult(null)
    try {
      const s = await api.getJarvisStatus()
      setTestResult(s)
    } catch (e) {
      setTestResult({ enabled: false, connected: false, error: e.message })
    } finally {
      setTesting(false)
    }
  }

  if (error) {
    return (
      <div className="max-w-2xl mx-auto glass rounded-xl p-6">
        <p className="text-jarvis-red text-sm">Failed to load settings: {error}</p>
        <button onClick={load} className="mt-3 text-xs text-jarvis-cyan-bright">
          Retry
        </button>
      </div>
    )
  }

  if (!settings) {
    return (
      <div className="flex justify-center py-16 text-jarvis-muted">
        <Loader2 size={20} className="animate-spin" />
      </div>
    )
  }

  const modelOptions = (current) => {
    const opts = new Set(models)
    if (current) opts.add(current)
    return Array.from(opts)
  }

  return (
    <div className="max-w-2xl mx-auto space-y-6 animate-fade-in">
      <h1 className="text-lg font-semibold text-white">Settings</h1>

      <section className="glass rounded-2xl p-5 space-y-4">
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted">Ollama Models</h2>
        <Field label="Fast model (quick confirmations)">
          <select value={settings.fast_model || ''} onChange={(e) => update('fast_model', e.target.value)} className={inputClass}>
            {modelOptions(settings.fast_model).map((m) => (
              <option key={m} value={m}>
                {m}
              </option>
            ))}
          </select>
        </Field>
        <Field label="Heavy model (batch processing)">
          <select value={settings.heavy_model || ''} onChange={(e) => update('heavy_model', e.target.value)} className={inputClass}>
            {modelOptions(settings.heavy_model).map((m) => (
              <option key={m} value={m}>
                {m}
              </option>
            ))}
          </select>
        </Field>
      </section>

      <section className="glass rounded-2xl p-5 space-y-4">
        <h2 className="text-xs uppercase tracking-widest text-jarvis-muted">MQTT Broker</h2>
        <div className="grid grid-cols-2 gap-4">
          <Field label="Host">
            <input type="text" value={settings.mqtt_host || ''} onChange={(e) => update('mqtt_host', e.target.value)} className={inputClass} />
          </Field>
          <Field label="Port">
            <input type="number" value={settings.mqtt_port || ''} onChange={(e) => update('mqtt_port', e.target.value)} className={inputClass} />
          </Field>
        </div>
      </section>

      <section className="glass rounded-2xl p-5 space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-xs uppercase tracking-widest text-jarvis-muted">JARVIS 3.0 Integration</h2>
          <label className="flex items-center gap-2 text-sm text-white cursor-pointer">
            <input
              type="checkbox"
              checked={!!settings.jarvis_enabled}
              onChange={(e) => update('jarvis_enabled', e.target.checked)}
              className="accent-cyan-500"
            />
            Enabled
          </label>
        </div>
        <Field label="Base URL">
          <input
            type="text"
            value={settings.jarvis_base_url || ''}
            onChange={(e) => update('jarvis_base_url', e.target.value)}
            placeholder="http://localhost:8000"
            className={inputClass}
            disabled={!settings.jarvis_enabled}
          />
        </Field>
        <div className="flex items-center gap-3">
          <button
            onClick={testConnection}
            disabled={testing}
            className="text-xs bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-3 py-1.5 hover:bg-jarvis-cyan/25 disabled:opacity-50"
          >
            {testing ? 'Testing…' : 'Test Connection'}
          </button>
          {testResult && (
            <span className={`flex items-center gap-1.5 text-xs ${testResult.connected ? 'text-jarvis-green' : 'text-jarvis-red'}`}>
              {testResult.connected ? <CheckCircle2 size={13} /> : <XCircle size={13} />}
              {testResult.connected ? 'Connected' : testResult.error || 'Unreachable'}
            </span>
          )}
        </div>
      </section>

      <div className="flex items-center gap-3">
        <button
          onClick={save}
          className="bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-5 py-2 text-sm hover:bg-jarvis-cyan/25"
        >
          Save Settings
        </button>
        {status && <span className="text-xs text-jarvis-muted">{status}</span>}
      </div>
    </div>
  )
}
