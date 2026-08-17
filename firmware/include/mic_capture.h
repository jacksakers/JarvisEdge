// Project  : Jarvis Edge Node
// File     : mic_capture.h
// Purpose  : I2S mic -> WAV-on-SD streaming capture (unbounded duration)
// Depends  : sd_card.h, <driver/i2s.h>, <SD.h>

#pragma once

/* ── Mic hardware (Elecrow CrowPanel Advance 3.5") ──────────────────────────
 *   Per Elecrow's own repo (readme "Version update points", v1.2+ boards):
 *   "i2s_mic changed to two pins IO9 and IO10". The old GPIO3 clock pin
 *   here was wrong — it left the PDM clock floating, which is why captures
 *   read one or two real samples and then pinned at -32768 for the rest.
 * ───────────────────────────────────────────────────────────────────────── */
#define MIC_I2S_WS      9    /* PDM clock */
#define MIC_I2S_DATA   10    /* PDM data */
#define MIC_SAMPLE_RATE 16000

// Installs the write-task/queue machinery. Call once from setup(), after
// sdCardInit(). Cheap — does not touch the I2S peripheral yet.
void micCaptureInit();

// Opens /queue/log_<timestamp>.wav and starts streaming captured audio to it.
// Returns false if the SD card isn't mounted, a recording is already active,
// or the I2S driver fails to install.
bool micCaptureStart();

// Call every loop() iteration while micCaptureIsActive() — drains whatever
// the I2S DMA buffer has ready (non-blocking) into the current chunk buffer.
void micCaptureHandle();

// Stops capture, flushes the final partial chunk, patches the WAV header,
// and closes the file. Blocks briefly (a few ms) for the writer task to
// finish — safe to call from the main loop, not from an ISR.
void micCaptureStop();

bool micCaptureIsActive();

// Samples the microphone briefly to check if voice/sound exceeds conversational threshold.
bool micCaptureDetectVAD();
