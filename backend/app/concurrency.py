"""
Global throttle for CPU/GPU-heavy AI work (docs/coding.txt 3.1 — non-blocking,
but concurrency still needs a ceiling).

faster-whisper transcription and both Ollama tiers are each expensive enough
that running more than one at a time can pin every CPU core and take the
whole host down — this happened in practice when a backlog of queued
recordings synced in quick succession and their /upload/audio requests (each
spawning its own ASR + fast-LLM + heavy-LLM work) overlapped. Serializing all
of it through one semaphore means requests queue up and run one at a time
instead of piling on concurrently.
"""
import asyncio

ai_semaphore = asyncio.Semaphore(1)
