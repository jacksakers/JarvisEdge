// Simple fetch wrapper for the JarvisEdge backend.
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
  return res.json()
}

export const api = {
  getSettings: () => request('/settings'),
  updateSettings: (body) => request('/settings', { method: 'PUT', body: JSON.stringify(body) }),
  getModels: () => request('/models'),
  getPrompts: () => request('/prompts'),
  updatePrompts: (body) => request('/prompts', { method: 'PUT', body: JSON.stringify(body) }),
  getLogs: (limit = 20) => request(`/logs?limit=${limit}`),
  getHealth: () => request('/health'),
}
