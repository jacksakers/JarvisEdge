import { useState } from 'react'
import SettingsPage from './pages/SettingsPage.jsx'
import PromptsPage from './pages/PromptsPage.jsx'
import DataPage from './pages/DataPage.jsx'
import { getBaseUrl, setBaseUrl } from './api.js'
import './App.css'

const TABS = [
  { id: 'settings', label: 'Settings', Component: SettingsPage },
  { id: 'prompts', label: 'Prompts', Component: PromptsPage },
  { id: 'data', label: 'Data', Component: DataPage },
]

function BackendUrlBar() {
  const [url, setUrl] = useState(getBaseUrl())
  const [saved, setSaved] = useState(false)

  function save() {
    setBaseUrl(url)
    setSaved(true)
    window.location.reload()
  }

  return (
    <div className="backend-bar">
      <label>
        Backend URL
        <input value={url} onChange={(e) => setUrl(e.target.value)} />
      </label>
      <button onClick={save}>Apply</button>
      {saved && <span className="status">Reloading…</span>}
    </div>
  )
}

function App() {
  const [activeTab, setActiveTab] = useState(TABS[0].id)
  const Active = TABS.find((t) => t.id === activeTab).Component

  return (
    <div className="app">
      <header>
        <h1>JarvisEdge Command Center</h1>
        <BackendUrlBar />
      </header>

      <nav className="tabs">
        {TABS.map((tab) => (
          <button
            key={tab.id}
            className={tab.id === activeTab ? 'active' : ''}
            onClick={() => setActiveTab(tab.id)}
          >
            {tab.label}
          </button>
        ))}
      </nav>

      <main>
        <Active />
      </main>
    </div>
  )
}

export default App
