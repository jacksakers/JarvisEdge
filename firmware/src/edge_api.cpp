// Project  : Jarvis Edge Node
// File     : edge_api.cpp
// Purpose  : Fire-and-forget HTTP helpers for talking to the Phase 3 backend
// Depends  : edge_api.h, settings.h, <HTTPClient.h>
//
// Every call spins up a short-lived FreeRTOS task on core 0 (mirrors
// sync_manager.cpp/mic_capture.cpp) so a slow/unreachable backend never
// stalls the UI thread. Requests are best-effort: failures are logged and
// otherwise ignored, since these are optimistic mirrors of an action the
// user already saw happen locally (strike-through, feed text, etc).

#include "edge_api.h"
#include "settings.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct ApiRequest {
    char path[80];
    char body[192];
};

static void api_post_task(void * pv)
{
    ApiRequest * req = (ApiRequest *)pv;

    if (WiFi.status() == WL_CONNECTED) {
        char url[144];
        snprintf(url, sizeof(url), "http://%s:%d%s",
                 settingsGetBackendHost(), settingsGetBackendPort(), req->path);

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int code = http.POST((uint8_t *)req->body, strlen(req->body));
        http.end();
        Serial.printf("[EdgeAPI] POST %s -> %d\n", req->path, code);
    } else {
        Serial.println("[EdgeAPI] Skipped POST — WiFi not connected.");
    }

    delete req;
    vTaskDelete(nullptr);
}

static void post_async(const char * path, const char * body)
{
    ApiRequest * req = new ApiRequest();
    strncpy(req->path, path, sizeof(req->path) - 1);
    req->path[sizeof(req->path) - 1] = '\0';
    strncpy(req->body, body, sizeof(req->body) - 1);
    req->body[sizeof(req->body) - 1] = '\0';

    xTaskCreatePinnedToCore(api_post_task, "edge_api_post", 6144, req, 1, nullptr, 0);
}

void edgeApiToggleFocus(int id)
{
    if (id < 0) return;
    char path[64];
    snprintf(path, sizeof(path), "/focus/%d/toggle", id);
    post_async(path, "{}");
}

void edgeApiTriggerAction(const char * action_type, const char * text)
{
    char path[64];
    snprintf(path, sizeof(path), "/actions/%s", action_type);

    // Minimal manual JSON escaping — text comes from the on-screen keyboard
    // and may contain quotes/backslashes.
    char escaped[160];
    size_t j = 0;
    for (size_t i = 0; text[i] != '\0' && j < sizeof(escaped) - 2; i++) {
        char c = text[i];
        if (c == '"' || c == '\\') escaped[j++] = '\\';
        if (j >= sizeof(escaped) - 1) break;
        escaped[j++] = c;
    }
    escaped[j] = '\0';

    char body[192];
    snprintf(body, sizeof(body), "{\"text\":\"%s\"}", escaped);
    post_async(path, body);
}
