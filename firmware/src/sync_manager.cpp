// Project  : Jarvis Edge Node
// File     : sync_manager.cpp
// Purpose  : Auto-sync loop — uploads queued recordings to the Phase 3 backend
//            and deletes them locally on success (docs/sdd.txt 4.2)
// Depends  : sync_manager.h, network_config.h, wifi_manager.h, plaud_mode.h,
//            sd_card.h, ui_status_bar.h, <SD.h>, <HTTPClient.h>
//
// Runs as its own FreeRTOS task pinned to core 0 (mirrors mic_capture.cpp's
// writer task) so a slow upload never blocks LVGL/touch on core 1. The task
// wakes periodically, and only touches the SD card when Plaud mode isn't
// actively recording (docs/coding.txt 2.2 — never contend for the SD bus).

#include "sync_manager.h"
#include "network_config.h"
#include "settings.h"
#include "wifi_manager.h"
#include "plaud_mode.h"
#include "sd_card.h"
#include "ui_status_bar.h"
#include "ui_screen_logs.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SYNC_POLL_MS      15000UL   /* how often the task wakes to scan /queue */
#define SYNC_BOUNDARY     "----JarvisEdgeBoundary7d1a"
/* ~4 min of 16kHz/16-bit mono — comfortably inside PSRAM, guards against a
 * runaway Plaud recording exhausting heap during upload. */
#define SYNC_MAX_FILE_BYTES  (8UL * 1024 * 1024)

// Whether a failed upload is worth retrying next cycle, or should just be
// dropped so a permanently-unprocessable file (e.g. transcription found no
// speech) doesn't clog /queue and get re-uploaded forever.
enum class UploadResult { Success, PermanentFailure, RetryLater };

// Belt-and-suspenders cap: whatever the backend's failure mode is (crash,
// hang, flaky network), a single file must never retry forever — that's
// what was pinning the backend's CPU and freezing the host. Tracked in RAM
// only (resets on reboot), which is fine since a fresh boot deserves a
// fresh attempt count.
#define SYNC_MAX_RETRIES   5
#define SYNC_TRACK_SLOTS   16
#define SYNC_INTER_UPLOAD_DELAY_MS  2000UL

struct RetryTrack {
    char name[40];
    uint8_t attempts;
};
static RetryTrack s_retry_track[SYNC_TRACK_SLOTS];

static uint8_t track_attempt(const String &name)
{
    RetryTrack * free_slot = nullptr;
    for (auto &slot : s_retry_track) {
        if (slot.name[0] == '\0') { if (!free_slot) free_slot = &slot; continue; }
        if (name.equals(slot.name)) {
            slot.attempts++;
            return slot.attempts;
        }
    }
    if (!free_slot) free_slot = &s_retry_track[0];   // table full — reuse oldest slot
    strncpy(free_slot->name, name.c_str(), sizeof(free_slot->name) - 1);
    free_slot->name[sizeof(free_slot->name) - 1] = '\0';
    free_slot->attempts = 1;
    return 1;
}

static void track_clear(const String &name)
{
    for (auto &slot : s_retry_track) {
        if (name.equals(slot.name)) { slot.name[0] = '\0'; break; }
    }
}

static UploadResult upload_one(const String &path, size_t file_len)
{
    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) return UploadResult::RetryLater;

    String filename = path.substring(path.lastIndexOf('/') + 1);
    String head = "--" SYNC_BOUNDARY "\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n"
                  "Content-Type: audio/wav\r\n\r\n";
    const char * tail = "\r\n--" SYNC_BOUNDARY "--\r\n";
    size_t tail_len = strlen(tail);
    size_t total_len = head.length() + file_len + tail_len;

    uint8_t * body = (uint8_t *)ps_malloc(total_len);
    if (!body) {
        Serial.println("[Sync] PSRAM alloc failed — will retry next cycle.");
        f.close();
        return UploadResult::RetryLater;
    }

    memcpy(body, head.c_str(), head.length());
    size_t read_bytes = f.read(body + head.length(), file_len);
    f.close();
    memcpy(body + head.length() + read_bytes, tail, tail_len);

    char url[96];
    snprintf(url, sizeof(url), "http://%s:%d/upload/audio", settingsGetBackendHost(), settingsGetBackendPort());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" SYNC_BOUNDARY);
    // Default 5s read timeout is far too short — the backend blocks on a
    // CPU-bound Whisper transcription plus a fast-tier LLM call before
    // responding, which routinely takes longer than that (HTTP -11 retries).
    http.setTimeout(60000);
    http.setConnectTimeout(10000);
    int code = http.POST(body, head.length() + read_bytes + tail_len);
    String response_body = "";
    if (code == 200) {
        response_body = http.getString();
    }
    http.end();
    free(body);

    if (code == 200) {
        Serial.printf("[Sync] Uploaded %s (HTTP 200).\n", filename.c_str());
        
        JsonDocument res_doc;
        if (deserializeJson(res_doc, response_body) == DeserializationError::Ok) {
            const char * transcript = res_doc["transcript"] | "";
            if (transcript && transcript[0] != '\0') {
                time_t now = time(nullptr);
                struct tm timeinfo;
                char time_str[32] = "NTP Unsynced";
                if (now > 1000000000L) {
                    localtime_r(&now, &timeinfo);
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
                }
                if (SD.exists("/logs") || SD.mkdir("/logs")) {
                    File log_f = SD.open("/logs/history.txt", FILE_APPEND);
                    if (log_f) {
                        log_f.printf("[%s] %s\n", time_str, transcript);
                        log_f.close();
                        Serial.println("[Sync] Saved transcript to SD card: /logs/history.txt");
                        uiLogsScreenReload();
                    }
                }
            }
        }
        return UploadResult::Success;
    }
    // 4xx means the backend rejected this specific file for good (e.g. empty
    // upload, or ASR found no speech in it) — retrying won't change that.
    if (code >= 400 && code < 500) {
        Serial.printf("[Sync] Upload rejected for %s: HTTP %d — discarding.\n", filename.c_str(), code);
        return UploadResult::PermanentFailure;
    }
    Serial.printf("[Sync] Upload failed for %s: HTTP %d (will retry)\n", filename.c_str(), code);
    return UploadResult::RetryLater;
}

static void sync_task(void * pv)
{
    (void)pv;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYNC_POLL_MS));

        if (!wifiManagerIsConnected() || !sdCardMounted() || plaudModeIsActive()) continue;

        File dir = SD.open("/queue");
        if (!dir) continue;

        for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
            if (f.isDirectory()) { f.close(); continue; }

            String name = String(f.name());
            if (!name.startsWith("/")) name = "/queue/" + name;
            size_t len = f.size();
            f.close();

            if (plaudModeIsActive()) break;   /* user started recording mid-sync — bail */
            if (len == 0 || len > SYNC_MAX_FILE_BYTES) {
                Serial.printf("[Sync] Skipping %s (size=%u out of bounds)\n", name.c_str(), (unsigned)len);
                continue;
            }

            UploadResult result = upload_one(name, len);
            if (result == UploadResult::RetryLater) {
                uint8_t attempts = track_attempt(name);
                if (attempts >= SYNC_MAX_RETRIES) {
                    Serial.printf("[Sync] %s failed %u times — giving up, discarding.\n", name.c_str(), attempts);
                    result = UploadResult::PermanentFailure;
                }
            }
            if (result != UploadResult::RetryLater) {
                SD.remove(name.c_str());
                track_clear(name);
            }

            // Never hammer the backend back-to-back, even when catching up
            // on a large backlog — gives the CPU-bound ASR/LLM pipeline room
            // to breathe between requests.
            vTaskDelay(pdMS_TO_TICKS(SYNC_INTER_UPLOAD_DELAY_MS));
        }
        dir.close();
        uiStatusBarSetQueueCount(sdCardCountQueueFiles());
    }
}

void syncManagerInit()
{
    xTaskCreatePinnedToCore(sync_task, "sync_task", 8192, nullptr, 1, nullptr, 0);
    Serial.println("[Sync] Auto-sync task armed (core 0).");
}
