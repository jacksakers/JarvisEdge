// Project  : Jarvis Edge Node
// File     : mic_capture.cpp
// Purpose  : I2S mic -> PSRAM buffer -> single WAV write on stop
// Depends  : mic_capture.h, sd_card.h
//
// The CrowPanel's onboard mic and its SD card slot share hardware and
// cannot be driven at the same time (per Elecrow's own docs/GitHub for
// this board) — the old double-buffered "stream to SD while I2S runs"
// design silently corrupted recordings via that contention. Capture now
// accumulates entirely into a PSRAM buffer while I2S is active and only
// touches the SD card once, after i2s_mic_uninstall() on stop, so the
// two never overlap.

#include "mic_capture.h"
#include "sd_card.h"
#include <Arduino.h>
#include <SD.h>
#include <driver/i2s.h>

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

#define READ_CHUNK_BYTES  4096   /* scratch buffer for pulling data out of the I2S DMA queue */

/* This board's PSRAM is 8MB total, shared with WiFi/TLS buffers and heap
 * overhead — requesting anywhere near all of it made ps_malloc() fail
 * (silently aborting recording — the symptom was "BOOT doesn't seem to
 * start capture"). ~100s of 16kHz/16-bit mono leaves plenty of headroom. */
#define MIC_MAX_RECORD_BYTES  (3UL * 1024 * 1024)

/* ESP32-S3 PDM RX decimation filter outputs samples at a fraction of full
 * scale — recordings come out sounding like near-silence with only the
 * loudest transients audible ("pops") unless amplified in software. Start
 * conservative and raise if voice is still too quiet; watch for clipping. */
#define MIC_DIGITAL_GAIN  8

static uint8_t   s_read_buf[READ_CHUNK_BYTES];
static uint8_t * s_record_buf   = nullptr;   /* PSRAM — whole recording, written to SD only after stop */
static char      s_record_path[48];
static uint32_t  s_audio_bytes  = 0;
static volatile bool s_active   = false;

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

static void write_wav_header(File &f, uint32_t audio_bytes)
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

    f.write((const uint8_t *)&h, sizeof(h));
}

// ── Public API ──────────────────────────────────────────────────────────

void micCaptureInit()
{
    Serial.println("[Mic] Capture module ready.");
}

bool micCaptureStart()
{
    if (s_active) return false;
    if (!sdCardMounted()) {
        Serial.println("[Mic] Cannot start: SD card not mounted.");
        return false;
    }

    if (!s_record_buf) {
        s_record_buf = (uint8_t *)ps_malloc(MIC_MAX_RECORD_BYTES);
        if (!s_record_buf) {
            Serial.printf("[Mic] PSRAM alloc of %u bytes failed (free PSRAM: %u).\n",
                          (unsigned)MIC_MAX_RECORD_BYTES, (unsigned)ESP.getFreePsram());
            return false;
        }
    }

    snprintf(s_record_path, sizeof(s_record_path), "/queue/log_%lu.wav", millis());
    s_audio_bytes = 0;

    // Mic and SD card share hardware on this board and can't be driven at
    // once — SD isn't touched again until i2s_mic_uninstall() in Stop().
    if (!i2s_mic_install()) return false;

    s_active = true;
    Serial.printf("[Mic] Recording (to PSRAM) for %s\n", s_record_path);
    return true;
}

void micCaptureHandle()
{
    if (!s_active) return;

    size_t bytes_read = 0;
    /* Non-blocking: 0 ms timeout — grab whatever's in the DMA buffer now. */
    i2s_read(MIC_I2S_PORT, s_read_buf, sizeof(s_read_buf), &bytes_read, 0);
    if (bytes_read == 0) return;

    apply_gain(s_read_buf, bytes_read);

    size_t space = MIC_MAX_RECORD_BYTES - s_audio_bytes;
    if (space == 0) return;   // hit the cap — drop further audio, keep what we have

    size_t copy_len = bytes_read < space ? bytes_read : space;
    memcpy(s_record_buf + s_audio_bytes, s_read_buf, copy_len);
    s_audio_bytes += copy_len;
}

void micCaptureStop()
{
    if (!s_active) return;

    i2s_mic_uninstall();   // release the mic before the SD write below

    File f = SD.open(s_record_path, FILE_WRITE);
    if (f) {
        write_wav_header(f, s_audio_bytes);
        f.write(s_record_buf, s_audio_bytes);
        f.close();
    } else {
        Serial.printf("[Mic] Failed to open %s for writing.\n", s_record_path);
    }

    s_active = false;
    Serial.printf("[Mic] Recording stopped (%u bytes audio).\n", (unsigned)s_audio_bytes);
}

bool micCaptureIsActive()
{
    return s_active;
}
