import { BrowserRouter, Routes, Route } from 'react-router-dom'
import AppShell from './components/AppShell.jsx'
import DashboardPage from './pages/DashboardPage.jsx'
import TodoPage from './pages/TodoPage.jsx'
import TapoPage from './pages/TapoPage.jsx'
import LogsPage from './pages/LogsPage.jsx'
import JarvisPage from './pages/JarvisPage.jsx'
import PromptsPage from './pages/PromptsPage.jsx'
import SettingsPage from './pages/SettingsPage.jsx'

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route element={<AppShell />}>
          <Route index element={<DashboardPage />} />
          <Route path="todo" element={<TodoPage />} />
          <Route path="tapo" element={<TapoPage />} />
          <Route path="logs" element={<LogsPage />} />
          <Route path="jarvis" element={<JarvisPage />} />
          <Route path="prompts" element={<PromptsPage />} />
          <Route path="settings" element={<SettingsPage />} />
        </Route>
      </Routes>
    </BrowserRouter>
  )
}

export default App
