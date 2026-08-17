import { useEffect, useState, useCallback } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Plus, Trash2, Check, GripVertical, Loader2 } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

function FocusRow({ item, onToggle, onDelete, onEdit }) {
  const [editing, setEditing] = useState(false)
  const [text, setText] = useState(item.text)

  function save() {
    setEditing(false)
    if (text.trim() && text.trim() !== item.text) onEdit(item.id, text.trim())
    else setText(item.text)
  }

  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, x: -20 }}
      className={clsx(
        'group flex items-center gap-3 glass rounded-xl px-4 py-3',
        item.done && 'opacity-50',
      )}
    >
      <GripVertical size={14} className="text-jarvis-muted/40 shrink-0" />
      <button
        onClick={() => onToggle(item.id)}
        className={clsx(
          'w-5 h-5 rounded-full border flex items-center justify-center shrink-0 transition-colors',
          item.done ? 'bg-jarvis-green border-jarvis-green' : 'border-jarvis-muted hover:border-jarvis-cyan',
        )}
      >
        {item.done && <Check size={12} className="text-black" />}
      </button>

      {editing ? (
        <input
          autoFocus
          value={text}
          onChange={(e) => setText(e.target.value)}
          onBlur={save}
          onKeyDown={(e) => e.key === 'Enter' && save()}
          className="flex-1 bg-jarvis-bg border border-jarvis-cyan/40 rounded px-2 py-1 text-sm text-white outline-none"
        />
      ) : (
        <span
          onClick={() => setEditing(true)}
          className={clsx(
            'flex-1 text-sm cursor-text',
            item.done ? 'line-through text-jarvis-muted' : 'text-white',
          )}
        >
          {item.text}
        </span>
      )}

      <span
        className={clsx(
          'text-[10px] px-1.5 py-0.5 rounded-full border shrink-0',
          item.source === 'ai'
            ? 'text-jarvis-purple border-jarvis-purple/30 bg-jarvis-purple/10'
            : 'text-jarvis-muted border-jarvis-border',
        )}
      >
        {item.source === 'ai' ? 'AI' : 'manual'}
      </span>

      <button
        onClick={() => onDelete(item.id)}
        className="text-jarvis-muted hover:text-jarvis-red opacity-0 group-hover:opacity-100 transition-opacity shrink-0"
      >
        <Trash2 size={14} />
      </button>
    </motion.div>
  )
}

export default function FocusPage() {
  const [items, setItems] = useState([])
  const [loading, setLoading] = useState(true)
  const [newText, setNewText] = useState('')
  const [error, setError] = useState('')

  const load = useCallback(async () => {
    try {
      setItems(await api.getFocus())
      setError('')
    } catch (e) {
      setError(e.message)
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    load()
    const id = setInterval(load, 8000)
    return () => clearInterval(id)
  }, [load])

  async function addItem() {
    const text = newText.trim()
    if (!text) return
    setNewText('')
    const created = await api.createFocus(text)
    setItems((prev) => [created, ...prev])
  }

  async function toggleItem(id) {
    setItems((prev) => prev.map((i) => (i.id === id ? { ...i, done: !i.done } : i)))
    await api.toggleFocus(id)
  }

  async function editItem(id, text) {
    setItems((prev) => prev.map((i) => (i.id === id ? { ...i, text } : i)))
    await api.updateFocus(id, { text })
  }

  async function deleteItem(id) {
    setItems((prev) => prev.filter((i) => i.id !== id))
    await api.deleteFocus(id)
  }

  const undone = items.filter((i) => !i.done)
  const done = items.filter((i) => i.done)

  return (
    <div className="max-w-2xl mx-auto space-y-6 animate-fade-in">
      <div>
        <h1 className="text-lg font-semibold text-white">Daily Focus</h1>
        <p className="text-xs text-jarvis-muted mt-1">
          Top 3 undone items are pushed to the device's Daily Focus tile over MQTT. Tapping a row on-device
          syncs back here automatically.
        </p>
      </div>

      <div className="flex gap-2">
        <input
          value={newText}
          onChange={(e) => setNewText(e.target.value)}
          onKeyDown={(e) => e.key === 'Enter' && addItem()}
          placeholder="Add a focus item…"
          className="flex-1 bg-jarvis-surface border border-jarvis-border rounded-lg px-3 py-2 text-sm text-white outline-none focus:border-jarvis-cyan/50"
        />
        <button
          onClick={addItem}
          className="bg-jarvis-cyan/15 border border-jarvis-cyan/40 text-jarvis-cyan-bright rounded-lg px-4 hover:bg-jarvis-cyan/25 flex items-center gap-1.5 text-sm"
        >
          <Plus size={15} /> Add
        </button>
      </div>

      {error && <p className="text-xs text-jarvis-red">{error}</p>}
      {loading && (
        <div className="flex justify-center py-8 text-jarvis-muted">
          <Loader2 size={20} className="animate-spin" />
        </div>
      )}

      {!loading && items.length === 0 && (
        <p className="text-sm text-jarvis-muted text-center py-8">No focus items yet — add one above.</p>
      )}

      <div className="space-y-2">
        <AnimatePresence initial={false}>
          {undone.map((item) => (
            <FocusRow key={item.id} item={item} onToggle={toggleItem} onDelete={deleteItem} onEdit={editItem} />
          ))}
        </AnimatePresence>
      </div>

      {done.length > 0 && (
        <div className="space-y-2 pt-2 border-t border-jarvis-border">
          <h2 className="text-[11px] uppercase tracking-widest text-jarvis-muted pt-2">Completed</h2>
          <AnimatePresence initial={false}>
            {done.map((item) => (
              <FocusRow key={item.id} item={item} onToggle={toggleItem} onDelete={deleteItem} onEdit={editItem} />
            ))}
          </AnimatePresence>
        </div>
      )}
    </div>
  )
}
