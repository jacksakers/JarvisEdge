// Project  : House Phone
// File     : tapo_control.cpp
// Purpose  : Ambient Home tile's backend client
// Depends  : tapo_control.h, settings.h, ui_screen_home.h

#include "tapo_control.h"
#include "settings.h"
#include "ui_screen_home.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAPO_POLL_MS  4000UL

static void backend_url(char * out, size_t out_len, const char * path)
{
    snprintf(out, out_len, "http://%s:%d%s",
             settingsGetBackendHost(), settingsGetBackendPort(), path);
}

void tapoControlInit()
{
    Serial.println("[Tapo] Ambient Home client armed.");
}

void tapoControlHandle(unsigned long now)
{
    static unsigned long s_last_poll = 0;
    if (now - s_last_poll < TAPO_POLL_MS) return;
    s_last_poll = now;

    if (WiFi.status() != WL_CONNECTED) return;

    char url[144];
    backend_url(url, sizeof(url), "/tapo/zones");

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return;
    }

    // Small, bounded doc — the backend never sends more than a handful of
    // zones (docs/new_idea.txt's grid is meant to be at-a-glance, not a list).
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.printf("[Tapo] JSON parse failed: %s\n", err.c_str());
        return;
    }

    static TapoZoneState zones[TAPO_MAX_ZONES];
    JsonArrayConst arr = doc.as<JsonArrayConst>();
    int count = 0;
    for (JsonObjectConst z : arr) {
        if (count >= TAPO_MAX_ZONES) break;
        TapoZoneState & slot = zones[count];
        slot.id = z["id"] | -1;
        strncpy(slot.name, z["name"] | "?", sizeof(slot.name) - 1);
        slot.name[sizeof(slot.name) - 1] = '\0';
        strncpy(slot.room, z["room"] | "", sizeof(slot.room) - 1);
        slot.room[sizeof(slot.room) - 1] = '\0';
        slot.on = z["on"] | false;
        slot.brightness = z["brightness"] | 100;
        slot.reachable = z["reachable"] | false;
        count++;
    }

    uiHomeSetZones(zones, count);
}

// ── Fire-and-forget POSTs (mirrors the old edge_api.cpp pattern) ───────────

struct TapoRequest {
    char path[64];
    char body[32];
};

static void tapo_post_task(void * pv)
{
    TapoRequest * req = (TapoRequest *)pv;

    if (WiFi.status() == WL_CONNECTED) {
        char url[160];
        snprintf(url, sizeof(url), "http://%s:%d%s",
                 settingsGetBackendHost(), settingsGetBackendPort(), req->path);

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int code = http.POST((uint8_t *)req->body, strlen(req->body));
        http.end();
        Serial.printf("[Tapo] POST %s -> %d\n", req->path, code);
    } else {
        Serial.println("[Tapo] Skipped POST — WiFi not connected.");
    }

    delete req;
    vTaskDelete(nullptr);
}

static void post_async(const char * path, const char * body)
{
    TapoRequest * req = new TapoRequest();
    strncpy(req->path, path, sizeof(req->path) - 1);
    req->path[sizeof(req->path) - 1] = '\0';
    strncpy(req->body, body, sizeof(req->body) - 1);
    req->body[sizeof(req->body) - 1] = '\0';

    xTaskCreatePinnedToCore(tapo_post_task, "tapo_post", 6144, req, 1, nullptr, 0);
}

void tapoControlToggle(int zone_id)
{
    if (zone_id < 0) return;
    char path[64];
    snprintf(path, sizeof(path), "/tapo/zones/%d/toggle", zone_id);
    post_async(path, "{}");
}

void tapoControlSetBrightness(int zone_id, int brightness)
{
    if (zone_id < 0) return;
    char path[64];
    snprintf(path, sizeof(path), "/tapo/zones/%d/brightness", zone_id);
    char body[32];
    snprintf(body, sizeof(body), "{\"brightness\":%d}", brightness);
    post_async(path, body);
}

void tapoControlAllOff()
{
    post_async("/tapo/zones/all_off", "{}");
}
