import { useState } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { ChevronDown, Trash2, Edit3, Save, X } from 'lucide-react'
import clsx from 'clsx'
import { api } from '../api.js'

const STATUS_META = {
  pending: { label: 'Pending', color: 'text-jarvis-amber border-jarvis-amber/30 bg-jarvis-amber/10' },
  fast_done: { label: 'Fast done', color: 'text-jarvis-cyan border-jarvis-cyan/30 bg-jarvis-cyan/10' },
  processed: { label: 'Processed', color: 'text-jarvis-green border-jarvis-green/30 bg-jarvis-green/10' },
  failed: { label: 'Failed', color: 'text-jarvis-red border-jarvis-red/30 bg-jarvis-red/10' },
}

export default function LogRow({ log, onDelete, onUpdate }) {
  const [open, setOpen] = useState(false)
  const [isEditing, setIsEditing] = useState(false)
  const [editedText, setEditedText] = useState(log.raw_text)
  const [editedResponse, setEditedResponse] = useState(log.fast_response)
  const meta = STATUS_META[log.status] || STATUS_META.pending

  const handleSave = async (e) => {
    e.stopPropagation()
    try {
      await onUpdate(log.id, { raw_text: editedText, fast_response: editedResponse })
      setIsEditing(false)
    } catch (err) {
      alert('Failed to update log: ' + err.message)
    }
  }

  return (
    <motion.div layout initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="glass rounded-xl overflow-hidden">
      <button
        onClick={() => setOpen((v) => !v)}
        className="w-full flex items-center gap-3 px-4 py-3 text-left"
      >
        <ChevronDown size={14} className={clsx('text-jarvis-muted transition-transform shrink-0', open && 'rotate-180')} />
        <div className="flex-1 min-w-0">
          <div className="text-sm text-white truncate">{log.raw_text || <em className="text-jarvis-muted">(no transcript)</em>}</div>
          <div className="text-[11px] text-jarvis-muted">{new Date(log.created_at).toLocaleString()}</div>
        </div>
        <span className={clsx('text-[10px] px-2 py-0.5 rounded-full border shrink-0', meta.color)}>{meta.label}</span>
        <button
          onClick={(e) => {
            e.stopPropagation()
            onDelete(log.id)
          }}
          className="text-jarvis-muted hover:text-jarvis-red shrink-0 ml-2"
        >
          <Trash2 size={14} />
        </button>
      </button>

      <AnimatePresence>
        {open && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            className="overflow-hidden border-t border-jarvis-border"
          >
            <div className="px-4 py-3 space-y-3 text-xs">
              {isEditing ? (
                <div className="space-y-3 p-1">
                  <div>
                    <label className="text-jarvis-muted uppercase tracking-widest text-[10px] block mb-1">Transcript</label>
                    <textarea
                      value={editedText}
                      onChange={(e) => setEditedText(e.target.value)}
                      className="w-full bg-jarvis-bg border border-jarvis-border rounded-lg p-2 text-white outline-none text-xs focus:border-jarvis-cyan"
                      rows={3}
                    />
                  </div>
                  <div>
                    <label className="text-jarvis-muted uppercase tracking-widest text-[10px] block mb-1">Fast Response Confirmation</label>
                    <input
                      type="text"
                      value={editedResponse}
                      onChange={(e) => setEditedResponse(e.target.value)}
                      className="w-full bg-jarvis-bg border border-jarvis-border rounded-lg p-2 text-white outline-none text-xs focus:border-jarvis-cyan"
                    />
                  </div>
                  <div className="flex gap-2 justify-end">
                    <button
                      onClick={(e) => { e.stopPropagation(); setIsEditing(false); }}
                      className="flex items-center gap-1 px-3 py-1.5 bg-jarvis-surface border border-jarvis-border rounded text-[11px] text-jarvis-muted hover:text-white"
                    >
                      <X size={12} /> Cancel
                    </button>
                    <button
                      onClick={handleSave}
                      className="flex items-center gap-1 px-3 py-1.5 bg-jarvis-cyan text-black font-semibold rounded text-[11px] hover:bg-jarvis-cyan-bright"
                    >
                      <Save size={12} /> Save Edits
                    </button>
                  </div>
                </div>
              ) : (
                <>
                  {log.audio_path && (
                    <div>
                      <div className="text-jarvis-muted uppercase tracking-widest text-[10px] mb-1">Recorded audio</div>
                      <audio controls preload="none" src={api.getLogAudioUrl(log.id)} className="w-full h-8" />
                    </div>
                  )}
                  <div>
                    <div className="text-jarvis-muted uppercase tracking-widest text-[10px] mb-1">Fast response</div>
                    <div className="text-jarvis-text">{log.fast_response || '—'}</div>
                  </div>
                  {log.structured_data && (
                    <div>
                      <div className="text-jarvis-muted uppercase tracking-widest text-[10px] mb-1">Structured data</div>
                      <pre className="bg-jarvis-bg border border-jarvis-border rounded-lg p-3 overflow-x-auto text-jarvis-cyan-bright font-mono text-[11px]">
                        {JSON.stringify(log.structured_data, null, 2)}
                      </pre>
                    </div>
                  )}
                  {log.jarvis_task_id && (
                    <div className="text-jarvis-purple font-semibold">JARVIS task: {log.jarvis_task_id}</div>
                  )}
                  <div className="pt-2 border-t border-jarvis-border/30 flex justify-end">
                    <button
                      onClick={(e) => { e.stopPropagation(); setIsEditing(true); }}
                      className="flex items-center gap-1 px-2.5 py-1.5 bg-jarvis-surface border border-jarvis-border hover:bg-jarvis-border text-jarvis-cyan rounded text-[10px] uppercase tracking-wider"
                    >
                      <Edit3 size={11} /> Edit Transcript
                    </button>
                  </div>
                </>
              )}
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  )
}