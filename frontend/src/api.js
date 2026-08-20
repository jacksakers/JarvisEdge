// Fetch wrapper for the House Phone backend (../backend/app/main.py).
// The backend host is configurable at runtime (stored in localStorage) since
// this is a local-network admin tool and the backend IP/port can vary
// between deployments.

const DEFAULT_BASE_URL = 'http://localhost:8010'

export function getBaseUrl() {
  return localStorage.getItem('jarvis_backend_url') || DEFAULT_BASE_URL
}

export function setBaseUrl(url) {
  localStorage.setItem('jarvis_backend_url', url.replace(/\/+$/, ''))
}

async function request(path, options = {}) {
  const res = await fetch(`${getBaseUrl()}${path}`, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  })
  if (!res.ok) {
    const text = await res.text().catch(() => '')
    throw new Error(`${options.method || 'GET'} ${path} failed: ${res.status} ${text}`)
  }
  if (res.status === 204) return null
  return res.json()
}

export const api = {
  // System
  getHealth: () => request('/health'),
  getDeviceStatus: () => request('/device/status'),

  // Settings / Prompts
  getSettings: () => request('/settings'),
  updateSettings: (body) => request('/settings', { method: 'PUT', body: JSON.stringify(body) }),
  getModels: () => request('/models'),
  getPrompts: () => request('/prompts'),
  updatePrompts: (body) => request('/prompts', { method: 'PUT', body: JSON.stringify(body) }),
  // Voice logs
  getLogs: (limit = 20) => request(`/logs?limit=${limit}`),
  createLog: (body) => request('/logs', { method: 'POST', body: JSON.stringify(body) }),
  updateLog: (id, body) => request(`/logs/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
  deleteLog: (id) => request(`/logs/${id}`, { method: 'DELETE' }),
  clearLogs: () => request('/logs', { method: 'DELETE' }),
  getLogAudioUrl: (id) => `${getBaseUrl()}/logs/${id}/audio`,

  // Todo List (full CRUD — Command Center only, not shown on-device)
  getFocus: () => request('/focus'),
  createFocus: (text) => request('/focus', { method: 'POST', body: JSON.stringify({ text }) }),
  updateFocus: (id, body) => request(`/focus/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
  toggleFocus: (id) => request(`/focus/${id}/toggle`, { method: 'POST' }),
  deleteFocus: (id) => request(`/focus/${id}`, { method: 'DELETE' }),

  // Ambient Home (Tapo bulb zones)
  getTapoZones: () => request('/tapo/zones'),
  createTapoZone: (body) => request('/tapo/zones', { method: 'POST', body: JSON.stringify(body) }),
  updateTapoZone: (id, body) => request(`/tapo/zones/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
  deleteTapoZone: (id) => request(`/tapo/zones/${id}`, { method: 'DELETE' }),
  toggleTapoZone: (id) => request(`/tapo/zones/${id}/toggle`, { method: 'POST' }),
  setTapoBrightness: (id, brightness) =>
    request(`/tapo/zones/${id}/brightness`, { method: 'POST', body: JSON.stringify({ brightness }) }),
  tapoAllOff: () => request('/tapo/zones/all_off', { method: 'POST' }),

  // JARVIS 3.0 integration
  getJarvisStatus: () => request('/jarvis/status'),
  getJarvisFeed: (limit = 20) => request(`/jarvis/feed?limit=${limit}`),
}
