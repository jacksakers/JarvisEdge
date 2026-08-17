import { BrowserRouter, Routes, Route } from 'react-router-dom'
import AppShell from './components/AppShell.jsx'
import DashboardPage from './pages/DashboardPage.jsx'
import FocusPage from './pages/FocusPage.jsx'
import ActionsPage from './pages/ActionsPage.jsx'
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
          <Route path="focus" element={<FocusPage />} />
          <Route path="actions" element={<ActionsPage />} />
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
