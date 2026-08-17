import { useEffect, useState } from 'react'
import { api } from '../api.js'

export default function DataPage() {
  const [logs, setLogs] = useState(null)
  const [error, setError] = useState('')
  const [limit, setLimit] = useState(20)

  useEffect(() => {
    load()
  }, [limit])

  async function load() {
    setError('')
    try {
      setLogs(await api.getLogs(limit))
    } catch (e) {
      setError(e.message)
    }
  }

  if (error) {
    return (
      <div className="panel">
        <p className="error">Failed to load logs: {error}</p>
        <button onClick={load}>Retry</button>
      </div>
    )
  }

  if (!logs) return <p>Loading logs…</p>

  return (
    <div className="panel">
      <h2>Captured Data</h2>
      <div className="actions">
        <label>
          Show last
          <select value={limit} onChange={(e) => setLimit(Number(e.target.value))}>
            <option value={20}>20</option>
            <option value={50}>50</option>
            <option value={100}>100</option>
          </select>
        </label>
        <button onClick={load}>Refresh</button>
      </div>

      {logs.length === 0 && <p>No entries yet.</p>}

      <table className="logs-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>Created</th>
            <th>Status</th>
            <th>Raw text</th>
            <th>Fast response</th>
            <th>Structured data</th>
          </tr>
        </thead>
        <tbody>
          {logs.map((log) => (
            <tr key={log.id}>
              <td>{log.id}</td>
              <td>{log.created_at}</td>
              <td>{log.status}</td>
              <td>{log.raw_text}</td>
              <td>{log.fast_response}</td>
              <td>
                <pre>{log.structured_data ? JSON.stringify(log.structured_data, null, 2) : ''}</pre>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )
}
