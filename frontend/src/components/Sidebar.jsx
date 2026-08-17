import { NavLink } from 'react-router-dom'
import { Radio, ListTodo, Zap, ScrollText, Cpu, Settings, MessageSquareText } from 'lucide-react'
import clsx from 'clsx'

const NAV_ITEMS = [
  { to: '/', label: 'Jarvis Feed', icon: Radio, end: true },
  { to: '/focus', label: 'Daily Focus', icon: ListTodo },
  { to: '/actions', label: 'Action Grid', icon: Zap },
  { to: '/logs', label: 'Voice Logs', icon: ScrollText },
  { to: '/jarvis', label: 'JARVIS Link', icon: Cpu },
  { to: '/prompts', label: 'Prompts', icon: MessageSquareText },
  { to: '/settings', label: 'Settings', icon: Settings },
]

export default function Sidebar({ connected }) {
  return (
    <div className="flex flex-col h-full">
      <div className="px-5 py-6 flex items-center gap-3 border-b border-jarvis-border">
        <div className="relative w-9 h-9 rounded-lg bg-jarvis-cyan/10 border border-jarvis-cyan/30 flex items-center justify-center">
          <Cpu size={18} className="text-jarvis-cyan" />
          <span
            className={clsx(
              'absolute -bottom-1 -right-1 w-3 h-3 rounded-full border-2 border-jarvis-bg',
              connected ? 'bg-jarvis-green animate-pulse-glow' : 'bg-jarvis-red',
            )}
          />
        </div>
        <div>
          <div className="text-sm font-semibold text-white leading-tight">Jarvis Edge</div>
          <div className="text-[11px] text-jarvis-muted leading-tight">Command Center</div>
        </div>
      </div>

      <nav className="flex-1 px-3 py-4 space-y-1 overflow-y-auto">
        {NAV_ITEMS.map(({ to, label, icon: Icon, end }) => (
          <NavLink
            key={to}
            to={to}
            end={end}
            className={({ isActive }) =>
              clsx(
                'flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm transition-colors',
                isActive
                  ? 'bg-jarvis-cyan/10 border border-jarvis-cyan/25 text-white'
                  : 'border border-transparent text-jarvis-muted hover:text-white hover:bg-jarvis-surface',
              )
            }
          >
            <Icon size={16} className="shrink-0" />
            {label}
          </NavLink>
        ))}
      </nav>

      <div className="px-5 py-4 border-t border-jarvis-border text-[10px] text-jarvis-muted/70">
        Jarvis Edge Node &middot; Phase 6
      </div>
    </div>
  )
}
