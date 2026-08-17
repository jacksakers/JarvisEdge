// Project  : Jarvis Edge Node
// File     : mic_capture.cpp
// Purpose  : I2S mic -> WAV-on-SD streaming capture (unbounded duration)
// Depends  : mic_capture.h, sd_card.h
//
// Flow: micCaptureHandle() fills one of two chunk buffers via non-blocking
// i2s_read(). When a buffer fills, it's handed to a FreeRTOS writer task
// pinned to core 0 while the main loop keeps filling the other buffer —
// the "double buffering" pattern required by docs/coding.txt 2.2 so a slow
// SD write never causes a dropped/glitched I2S frame. On stop, the writer
// task patches the WAV header (audio length wasn't known up front) and
// closes the file, signalling completion via a semaphore.

#include "mic_capture.h"
#include "sd_card.h"
#include <Arduino.h>
#include <SD.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#define MIC_I2S_PORT  I2S_NUM_0   /* PDM mic only works on I2S0 on ESP32-S3 */

#pragma pack(push, 1)
struct WavHeader {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     format[4];
    char     sub1_id[4];
    uint32_t sub1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     sub2_id[4];
    uint32_t sub2_size;
};
#pragma pack(pop)
#define WAV_HEADER_SIZE  sizeof(WavHeader)

#define CHUNK_BYTES  4096   /* 2048 samples @ 16-bit mono, ~128 ms per chunk */

/* ESP32-S3 PDM RX decimation filter outputs samples at a fraction of full
 * scale — recordings come out sounding like near-silence with only the
 * loudest transients audible ("pops") unless amplified in software. Start
 * conservative and raise if voice is still too quiet; watch for clipping. */
#define MIC_DIGITAL_GAIN  8

struct WriteItem {
    uint8_t * data;
    size_t    len;
    bool      is_final;
};

static uint8_t         s_chunk_buf[2][CHUNK_BYTES];
static uint8_t          s_active_idx = 0;
static size_t           s_fill_len   = 0;

static QueueHandle_t     s_write_q   = nullptr;
static SemaphoreHandle_t s_done_sem  = nullptr;
static File              s_file;
static uint32_t          s_audio_bytes = 0;
static volatile bool     s_active   = false;

// ── I2S (PDM RX) ──────────────────────────────────────────────────────────

static bool i2s_mic_install()
{
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = 512,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0,
    };

    if (i2s_driver_install(MIC_I2S_PORT, &cfg, 0, nullptr) != ESP_OK) {
        Serial.println("[Mic] i2s_driver_install failed.");
        return false;
    }

    i2s_pin_config_t pins = {
        .mck_io_num   = I2S_PIN_NO_CHANGE,
        .bck_io_num   = I2S_PIN_NO_CHANGE,   /* PDM: no BCLK */
        .ws_io_num    = MIC_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_I2S_DATA,
    };
    if (i2s_set_pin(MIC_I2S_PORT, &pins) != ESP_OK) {
        Serial.println("[Mic] i2s_set_pin failed.");
        i2s_driver_uninstall(MIC_I2S_PORT);
        return false;
    }
    return true;
}

static void i2s_mic_uninstall()
{
    i2s_driver_uninstall(MIC_I2S_PORT);
}

// ── Digital gain (see MIC_DIGITAL_GAIN above) ─────────────────────────────

static void apply_gain(uint8_t * buf, size_t len)
{
    int16_t * samples = (int16_t *)buf;
    size_t count = len / sizeof(int16_t);
    for (size_t i = 0; i < count; i++) {
        int32_t amplified = (int32_t)samples[i] * MIC_DIGITAL_GAIN;
        if (amplified > INT16_MAX) amplified = INT16_MAX;
        else if (amplified < INT16_MIN) amplified = INT16_MIN;
        samples[i] = (int16_t)amplified;
    }
}

// ── WAV header ────────────────────────────────────────────────────────────

static void patch_wav_header(uint32_t audio_bytes)
{
    WavHeader h;
    memcpy(h.chunk_id, "RIFF", 4);
    h.chunk_size = 36 + audio_bytes;
    memcpy(h.format, "WAVE", 4);
    memcpy(h.sub1_id, "fmt ", 4);
    h.sub1_size       = 16;
    h.audio_format    = 1;
    h.num_channels    = 1;
    h.sample_rate     = MIC_SAMPLE_RATE;
    h.byte_rate       = MIC_SAMPLE_RATE * 2;
    h.block_align     = 2;
    h.bits_per_sample = 16;
    memcpy(h.sub2_id, "data", 4);
    h.sub2_size = audio_bytes;

    s_file.seek(0);
    s_file.write((const uint8_t *)&h, sizeof(h));
}

// ── Writer task (core 0) — owns the File handle exclusively ──────────────

static void writer_task(void * pv)
{
    (void)pv;
    WriteItem item;
    for (;;) {
        if (xQueueReceive(s_write_q, &item, portMAX_DELAY) != pdTRUE) continue;

        if (item.len > 0) {
            s_file.write(item.data, item.len);
            s_audio_bytes += item.len;
        }
        if (item.is_final) {
            patch_wav_header(s_audio_bytes);
            s_file.close();
            xSemaphoreGive(s_done_sem);
        }
    }
}

// ── Public API ──────────────────────────────────────────────────────────

void micCaptureInit()
{
    s_write_q  = xQueueCreate(4, sizeof(WriteItem));
    s_done_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(writer_task, "mic_writer", 4096, nullptr, 1, nullptr, 0);
    Serial.println("[Mic] Capture module ready.");
}

bool micCaptureStart()
{
    if (s_active) return false;
    if (!sdCardMounted()) {
        Serial.println("[Mic] Cannot start: SD card not mounted.");
        return false;
    }

    char path[48];
    snprintf(path, sizeof(path), "/queue/log_%lu.wav", millis());

    s_file = SD.open(path, FILE_WRITE);
    if (!s_file) {
        Serial.printf("[Mic] Failed to open %s\n", path);
        return false;
    }

    uint8_t placeholder[WAV_HEADER_SIZE] = {0};
    s_file.write(placeholder, sizeof(placeholder));   // patched on stop
    s_audio_bytes = 0;

    if (!i2s_mic_install()) {
        s_file.close();
        SD.remove(path);
        return false;
    }

    s_active_idx = 0;
    s_fill_len   = 0;
    s_active     = true;
    Serial.printf("[Mic] Recording to %s\n", path);
    return true;
}

void micCaptureHandle()
{
    if (!s_active) return;

    uint8_t * buf = s_chunk_buf[s_active_idx];
    size_t remaining = CHUNK_BYTES - s_fill_len;
    size_t bytes_read = 0;

    /* Non-blocking: 0 ms timeout — grab whatever's in the DMA buffer now. */
    i2s_read(MIC_I2S_PORT, buf + s_fill_len, remaining, &bytes_read, 0);
    apply_gain(buf + s_fill_len, bytes_read);
    s_fill_len += bytes_read;

    if (s_fill_len >= CHUNK_BYTES) {
        WriteItem item = { buf, CHUNK_BYTES, false };
        xQueueSend(s_write_q, &item, portMAX_DELAY);
        s_active_idx ^= 1;   // swap — capture keeps filling the other buffer
        s_fill_len = 0;
    }
}

void micCaptureStop()
{
    if (!s_active) return;

    i2s_mic_uninstall();

    WriteItem item = { s_chunk_buf[s_active_idx], s_fill_len, true };
    xQueueSend(s_write_q, &item, portMAX_DELAY);
    xSemaphoreTake(s_done_sem, portMAX_DELAY);

    s_active = false;
    Serial.printf("[Mic] Recording stopped (%u bytes audio).\n", (unsigned)s_audio_bytes);
}

bool micCaptureIsActive()
{
    return s_active;
}
