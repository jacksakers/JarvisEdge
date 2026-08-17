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
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SYNC_POLL_MS      15000UL   /* how often the task wakes to scan /queue */
#define SYNC_BOUNDARY     "----JarvisEdgeBoundary7d1a"
/* ~4 min of 16kHz/16-bit mono — comfortably inside PSRAM, guards against a
 * runaway Plaud recording exhausting heap during upload. */
#define SYNC_MAX_FILE_BYTES  (8UL * 1024 * 1024)

static bool upload_one(const String &path, size_t file_len)
{
    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) return false;

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
        return false;
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
    int code = http.POST(body, head.length() + read_bytes + tail_len);
    http.end();
    free(body);

    if (code == 200) {
        Serial.printf("[Sync] Uploaded %s (HTTP 200).\n", filename.c_str());
        return true;
    }
    Serial.printf("[Sync] Upload failed for %s: HTTP %d\n", filename.c_str(), code);
    return false;
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

            if (upload_one(name, len)) SD.remove(name.c_str());
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
