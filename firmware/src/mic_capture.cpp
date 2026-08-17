// Project  : Jarvis Edge Node
// File     : mic_capture.cpp
// Purpose  : I2S mic -> PSRAM buffer -> High-Pass DC Filter -> WAV write
// Depends  : mic_capture.h, sd_card.h

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

#define READ_CHUNK_BYTES      4096
#define MIC_MAX_RECORD_BYTES  (3UL * 1024 * 1024)

/* Adjust gain once DC offset is removed. Start at 4 or 8. */
#define MIC_DIGITAL_GAIN      4

/* Select Microphone Mode: 0 for Standard I2S MEMS mic (INMP441 / L330 / F1J1), 1 for PDM mic */
#ifndef MIC_TYPE_PDM
#define MIC_TYPE_PDM 0
#endif

/* Hardware Pin Mapping for Elecrow ESP32-S3 Board */
#ifndef MIC_I2S_BCK
#define MIC_I2S_BCK  9   /* Bit Clock (SCK) */
#endif
#ifndef MIC_I2S_WS
#define MIC_I2S_WS   3   /* Word Select / LRCLK */
#endif
#ifndef MIC_I2S_DATA
#define MIC_I2S_DATA 10  /* Serial Data Input (SD) */
#endif
#ifndef MIC_PWR_PIN
#define MIC_PWR_PIN  -1  /* Optional GPIO power enable pin (-1 if powered by 3V3 rail directly) */
#endif

enum ChannelSelect {
    CHAN_AUTO = 0,
    CHAN_LEFT,
    CHAN_RIGHT,
    CHAN_BOTH
};

static uint8_t   s_read_buf[READ_CHUNK_BYTES];
static uint8_t   s_mono_buf[READ_CHUNK_BYTES / 2];
static uint8_t * s_record_buf     = nullptr;
static char      s_record_path[48];
static uint32_t  s_audio_bytes    = 0;
static int16_t   s_peak_sample    = 0;
static volatile bool s_active     = false;

/* High-Pass Filter & Auto-Channel State */
static float         s_dc_estimate   = 0.0f;
static bool          s_dc_init       = false;
static ChannelSelect s_chan_mode     = CHAN_AUTO;
static uint32_t      s_last_debug_ms = 0;

// ── I2S (Standard I2S / PDM RX) ───────────────────────────────────────────

static bool i2s_mic_install()
{
    i2s_config_t cfg = {
#if MIC_TYPE_PDM
        /* ESP32-S3 PDM hardware RX mode requires I2S_MODE_PDM */
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
#else
        /* Standard I2S master receiver mode for MEMS microphones */
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = MIC_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT, /* Standard MEMS mic 24/32-bit slot */
#endif
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,  /* Sample both stereo slots to auto-detect active L/R channel */
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
#if MIC_TYPE_PDM
        .bck_io_num   = I2S_PIN_NO_CHANGE,   /* Unused in PDM mode */
        .ws_io_num    = MIC_I2S_BCK,         /* PDM CLK pin */
#else
        .bck_io_num   = MIC_I2S_BCK,         /* IO9 - Bit Clock / SCK */
        .ws_io_num    = MIC_I2S_WS,          /* IO3 - Word Select / WS */
#endif
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_I2S_DATA,        /* IO10 - Serial Data / SD */
    };

    if (i2s_set_pin(MIC_I2S_PORT, &pins) != ESP_OK) {
        Serial.println("[Mic] i2s_set_pin failed.");
        i2s_driver_uninstall(MIC_I2S_PORT);
        return false;
    }

    i2s_zero_dma_buffer(MIC_I2S_PORT);
    i2s_start(MIC_I2S_PORT);

    return true;
}

static void i2s_mic_uninstall()
{
    i2s_stop(MIC_I2S_PORT);
    i2s_driver_uninstall(MIC_I2S_PORT);
}

// ── Auto Channel Selection & 32-bit to 16-bit Conversion ─────────────────

static size_t deinterleave_pdm_samples(const uint8_t * src, size_t src_len, uint8_t * dst)
{
    int16_t * mono = (int16_t *)dst;

#if MIC_TYPE_PDM
    /* ESP32-S3 PDM Hardware RX outputs native 16-bit PCM samples */
    const int16_t * stereo16 = (const int16_t *)src;
    size_t pair_count = src_len / (2 * sizeof(int16_t));

    if (s_chan_mode == CHAN_AUTO && pair_count > 0) {
        int16_t l_min = INT16_MAX, l_max = INT16_MIN;
        int16_t r_min = INT16_MAX, r_max = INT16_MIN;

        for (size_t i = 0; i < pair_count; i++) {
            int16_t l = stereo16[2 * i];
            int16_t r = stereo16[2 * i + 1];

            if (l < l_min) l_min = l;
            if (l > l_max) l_max = l;
            if (r < r_min) r_min = r;
            if (r > r_max) r_max = r;
        }

        int32_t l_delta = l_max - l_min;
        int32_t r_delta = r_max - r_min;

        bool left_active  = l_delta > 10;
        bool right_active = r_delta > 10;

        if (left_active && !right_active) {
            s_chan_mode = CHAN_LEFT;
            Serial.printf("[Mic Diag] Auto-detected PDM Channel: LEFT (L/R=GND). Right silent.\n");
        } else if (right_active && !left_active) {
            s_chan_mode = CHAN_RIGHT;
            Serial.printf("[Mic Diag] Auto-detected PDM Channel: RIGHT (L/R=VDD). Left silent.\n");
        } else if (!left_active && !right_active) {
            s_chan_mode = CHAN_LEFT; /* Fallback to avoid averaging dead signals */
            Serial.printf("[Mic Diag] WARNING: BOTH CHANNELS DEAD (Flatline %d). Check wiring/MIC_TYPE_PDM!\n", l_min);
        } else {
            s_chan_mode = CHAN_BOTH;
            Serial.println("[Mic Diag] Auto-detected PDM Channel: BOTH channels active.");
        }
    }

    for (size_t i = 0; i < pair_count; i++) {
        int16_t l = stereo16[2 * i];
        int16_t r = stereo16[2 * i + 1];

        if (s_chan_mode == CHAN_RIGHT) {
            mono[i] = r;
        } else if (s_chan_mode == CHAN_BOTH) {
            mono[i] = (int16_t)(((int32_t)l + (int32_t)r) / 2);
        } else {
            mono[i] = l;
        }
    }

    return pair_count * sizeof(int16_t);

#else
    /* Standard 32-bit I2S mode (24-bit MEMS audio in 32-bit slots) */
    const int32_t * stereo32 = (const int32_t *)src;
    size_t pair_count = src_len / (2 * sizeof(int32_t));

    if (s_chan_mode == CHAN_AUTO && pair_count > 0) {
        int32_t l_min = INT32_MAX, l_max = INT32_MIN;
        int32_t r_min = INT32_MAX, r_max = INT32_MIN;

        for (size_t i = 0; i < pair_count; i++) {
            int32_t l = stereo32[2 * i] >> 14;
            int32_t r = stereo32[2 * i + 1] >> 14;

            if (l < l_min) l_min = l;
            if (l > l_max) l_max = l;
            if (r < r_min) r_min = r;
            if (r > r_max) r_max = r;
        }

        int32_t l_delta = l_max - l_min;
        int32_t r_delta = r_max - r_min;

        bool left_active  = l_delta > 10;
        bool right_active = r_delta > 10;

        if (left_active && !right_active) {
            s_chan_mode = CHAN_LEFT;
            Serial.printf("[Mic Diag] Auto-detected I2S Channel: LEFT (L/R=GND). Right silent.\n");
        } else if (right_active && !left_active) {
            s_chan_mode = CHAN_RIGHT;
            Serial.printf("[Mic Diag] Auto-detected I2S Channel: RIGHT (L/R=VDD). Left silent.\n");
        } else if (!left_active && !right_active) {
            s_chan_mode = CHAN_LEFT;
            Serial.printf("[Mic Diag] WARNING: BOTH CHANNELS DEAD (Flatline %d). Check wiring/MIC_TYPE_PDM!\n", (int)l_min);
        } else {
            s_chan_mode = CHAN_BOTH;
            Serial.println("[Mic Diag] Auto-detected I2S Channel: BOTH channels active.");
        }
    }

    for (size_t i = 0; i < pair_count; i++) {
        /* Convert 24-bit MEMS sample in 32-bit slot to 16-bit PCM sample */
        int32_t l32 = stereo32[2 * i] >> 14;
        int32_t r32 = stereo32[2 * i + 1] >> 14;

        int16_t l = (l32 > INT16_MAX) ? INT16_MAX : ((l32 < INT16_MIN) ? INT16_MIN : (int16_t)l32);
        int16_t r = (r32 > INT16_MAX) ? INT16_MAX : ((r32 < INT16_MIN) ? INT16_MIN : (int16_t)r32);

        if (s_chan_mode == CHAN_RIGHT) {
            mono[i] = r;
        } else if (s_chan_mode == CHAN_BOTH) {
            mono[i] = (int16_t)(((int32_t)l + (int32_t)r) / 2);
        } else {
            mono[i] = l;
        }
    }

    return pair_count * sizeof(int16_t);
#endif
}

// ── Real-time DC Removal & Gain Filter ────────────────────────────────────

static void process_audio_samples(uint8_t * buf, size_t len)
{
    int16_t * samples = (int16_t *)buf;
    size_t count = len / sizeof(int16_t);

    if (count == 0) return;

    /* Initialize DC estimate on first valid sample to avoid step-response pop */
    if (!s_dc_init) {
        s_dc_estimate = (float)samples[0];
        s_dc_init = true;
    }

    int16_t raw_min = INT16_MAX;
    int16_t raw_max = INT16_MIN;

    for (size_t i = 0; i < count; i++) {
        int16_t raw = samples[i];
        if (raw < raw_min) raw_min = raw;
        if (raw > raw_max) raw_max = raw;

        /* Exponential Moving Average to track and subtract DC offset */
        s_dc_estimate = (0.98f * s_dc_estimate) + (0.02f * (float)raw);
        float clean_ac = (float)raw - s_dc_estimate;

        /* Apply Digital Gain to pure AC signal */
        int32_t amplified = (int32_t)(clean_ac * MIC_DIGITAL_GAIN);
        if (amplified > INT16_MAX) amplified = INT16_MAX;
        else if (amplified < INT16_MIN) amplified = INT16_MIN;

        samples[i] = (int16_t)amplified;

        int16_t abs_sample = samples[i] < 0 ? (int16_t)-samples[i] : samples[i];
        if (abs_sample > s_peak_sample) s_peak_sample = abs_sample;
    }

    /* Print diagnostic info every 1 second while recording */
    if (millis() - s_last_debug_ms > 1000) {
        s_last_debug_ms = millis();
        if (raw_min == raw_max) {
#if MIC_TYPE_PDM
            Serial.printf("[Mic Diag WARNING] Flatline at %d! NO AUDIO. Check if mic is standard I2S (not PDM) or wiring.\n", raw_min);
#else
            Serial.printf("[Mic Diag WARNING] Flatline at %d! NO AUDIO. Check if mic is PDM (not standard I2S) or wiring.\n", raw_min);
#endif
        } else {
            Serial.printf("[Mic Diag] Raw Range: [%d to %d] | Est. DC Bias: %.1f | AC Peak: %d\n",
                          raw_min, raw_max, s_dc_estimate, s_peak_sample);
        }
    }
}

// ── WAV Header ────────────────────────────────────────────────────────────

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

// ── Public API ────────────────────────────────────────────────────────────

void micCaptureInit()
{
    /* Enable onboard microphone power/mux line via GPIO 45 (1 = MIC, 0 = WM) */
    pinMode(45, OUTPUT);
    digitalWrite(45, HIGH);
    Serial.println("[Mic] GPIO 45 pulled HIGH to enable onboard microphone.");

#if defined(MIC_PWR_PIN) && (MIC_PWR_PIN >= 0)
    pinMode(MIC_PWR_PIN, OUTPUT);
    digitalWrite(MIC_PWR_PIN, HIGH);
    Serial.printf("[Mic] Power pin GPIO %d pulled HIGH.\n", MIC_PWR_PIN);
#endif
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
            Serial.printf("[Mic] PSRAM alloc failed (free: %u).\n", (unsigned)ESP.getFreePsram());
            return false;
        }
    }

    snprintf(s_record_path, sizeof(s_record_path), "/queue/log_%lu.wav", millis());
    s_audio_bytes   = 0;
    s_peak_sample   = 0;
    s_dc_estimate   = 0.0f;
    s_dc_init       = false;
    s_chan_mode     = CHAN_AUTO;
    s_last_debug_ms = millis();

    if (!i2s_mic_install()) return false;

    s_active = true;
    Serial.printf("[Mic] Recording (to PSRAM) for %s\n", s_record_path);
    return true;
}

void micCaptureHandle()
{
    if (!s_active) return;

    size_t bytes_read = 0;
    i2s_read(MIC_I2S_PORT, s_read_buf, sizeof(s_read_buf), &bytes_read, 0);
    if (bytes_read == 0) return;

    /* De-interleave stereo input to mono & auto-detect active channel */
    size_t mono_len = deinterleave_pdm_samples(s_read_buf, bytes_read, s_mono_buf);

    /* Process DC removal and gain on mono channel */
    process_audio_samples(s_mono_buf, mono_len);

    size_t space = MIC_MAX_RECORD_BYTES - s_audio_bytes;
    if (space == 0) return;

    size_t copy_len = mono_len < space ? mono_len : space;
    memcpy(s_record_buf + s_audio_bytes, s_mono_buf, copy_len);
    s_audio_bytes += copy_len;
}

void micCaptureStop()
{
    if (!s_active) return;

    i2s_mic_uninstall();

    File f = SD.open(s_record_path, FILE_WRITE);
    if (f) {
        write_wav_header(f, s_audio_bytes);
        f.write(s_record_buf, s_audio_bytes);
        f.close();
    } else {
        Serial.printf("[Mic] Failed to open %s for writing.\n", s_record_path);
    }

    s_active = false;
    Serial.printf("[Mic] Recording stopped (%u bytes audio, AC peak sample %d/32767).\n",
                  (unsigned)s_audio_bytes, s_peak_sample);
}

bool micCaptureIsActive()
{
    return s_active;
}