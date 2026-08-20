// Project  : House Phone
// File     : settings.cpp
// Purpose  : Persistent key-value settings stored on SD card
// Depends  : settings.h, sd_card.h, network_config.h, <SD.h>

#include "settings.h"
#include "sd_card.h"
#include "network_config.h"
#include <Arduino.h>
#include <SD.h>

#define SETTINGS_FILE  "/settings/jarvis.txt"
#define MAX_LINE_LEN   96

static char s_wifi_ssid[64]     = "";
static char s_wifi_password[64] = "";
static char s_backend_host[64]  = "";
static int  s_backend_port      = 0;
static char s_mqtt_host[64]     = "";
static int  s_mqtt_port         = 0;

static bool s_ambient_vad       = false;
static bool s_power_saving      = false;
static int  s_screen_off        = 30;
static bool s_screen_lock       = false;

static int  s_alarm_hour        = -1;   // -1 = never configured
static int  s_alarm_minute      = 0;
static bool s_alarm_enabled     = false;


static void apply_kv(const char * key, const char * val)
{
    if (strcmp(key, "wifi_ssid") == 0) {
        strncpy(s_wifi_ssid, val, sizeof(s_wifi_ssid) - 1);
    } else if (strcmp(key, "wifi_password") == 0) {
        strncpy(s_wifi_password, val, sizeof(s_wifi_password) - 1);
    } else if (strcmp(key, "backend_host") == 0) {
        strncpy(s_backend_host, val, sizeof(s_backend_host) - 1);
    } else if (strcmp(key, "backend_port") == 0) {
        s_backend_port = atoi(val);
    } else if (strcmp(key, "mqtt_host") == 0) {
        strncpy(s_mqtt_host, val, sizeof(s_mqtt_host) - 1);
    } else if (strcmp(key, "mqtt_port") == 0) {
        s_mqtt_port = atoi(val);

    } else if (strcmp(key, "ambient_vad_mode") == 0) {
        s_ambient_vad = (atoi(val) != 0);
    } else if (strcmp(key, "power_saving_mode") == 0) {
        s_power_saving = (atoi(val) != 0);
    } else if (strcmp(key, "screen_off_timeout") == 0) {
        s_screen_off = atoi(val);
    } else if (strcmp(key, "screen_lock_enabled") == 0) {
        s_screen_lock = (atoi(val) != 0);
    } else if (strcmp(key, "alarm_hour") == 0) {
        s_alarm_hour = atoi(val);
    } else if (strcmp(key, "alarm_minute") == 0) {
        s_alarm_minute = atoi(val);
    } else if (strcmp(key, "alarm_enabled") == 0) {
        s_alarm_enabled = (atoi(val) != 0);
    }
    /* Unknown keys are silently ignored for forwards compatibility */
}

static void load_from_sd()
{
    if (!sdCardMounted()) return;

    File f = SD.open(SETTINGS_FILE, FILE_READ);
    if (!f) {
        Serial.println("[Settings] No settings file — using compile-time defaults.");
        return;
    }

    char line[MAX_LINE_LEN];
    while (f.available()) {
        int n = f.readBytesUntil('\n', line, (int)sizeof(line) - 1);
        line[n] = '\0';
        if (n > 0 && line[n - 1] == '\r') { line[n - 1] = '\0'; n--; }
        if (n == 0 || line[0] == '#') continue;

        char * eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        apply_kv(line, eq + 1);
    }
    f.close();
    Serial.println("[Settings] Loaded from SD.");
}

void settingsInit()
{
    s_wifi_ssid[0] = '\0';
    s_wifi_password[0] = '\0';
    s_backend_host[0] = '\0';
    s_backend_port = 0;
    s_mqtt_host[0] = '\0';
    s_mqtt_port = 0;

    load_from_sd();

    Serial.printf("[Settings] Applied: ssid=%s  backend=%s:%d  mqtt=%s:%d\n",
                  s_wifi_ssid[0] ? s_wifi_ssid : "(default)",
                  settingsGetBackendHost(), settingsGetBackendPort(),
                  settingsGetMqttHost(), settingsGetMqttPort());
}

void settingsSave()
{
    if (!sdCardMounted()) {
        Serial.println("[Settings] SD not mounted — save skipped.");
        return;
    }

    if (!SD.exists("/settings")) SD.mkdir("/settings");
    if (SD.exists(SETTINGS_FILE)) SD.remove(SETTINGS_FILE);

    File f = SD.open(SETTINGS_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[Settings] ERROR: Could not open settings file for write.");
        return;
    }

    f.printf("# Jarvis Edge Node settings — edited on-device\n");
    f.printf("wifi_ssid=%s\n",     s_wifi_ssid);
    f.printf("wifi_password=%s\n", s_wifi_password);
    f.printf("backend_host=%s\n", s_backend_host);
    f.printf("backend_port=%d\n", s_backend_port);
    f.printf("mqtt_host=%s\n",    s_mqtt_host);
    f.printf("mqtt_port=%d\n",    s_mqtt_port);
    f.printf("ambient_vad_mode=%d\n", s_ambient_vad ? 1 : 0);
    f.printf("power_saving_mode=%d\n", s_power_saving ? 1 : 0);
    f.printf("screen_off_timeout=%d\n", s_screen_off);
    f.printf("screen_lock_enabled=%d\n", s_screen_lock ? 1 : 0);
    f.printf("alarm_hour=%d\n", s_alarm_hour);
    f.printf("alarm_minute=%d\n", s_alarm_minute);
    f.printf("alarm_enabled=%d\n", s_alarm_enabled ? 1 : 0);
    f.close();

    Serial.println("[Settings] Saved.");
}

const char * settingsGetWifiSSID()
{
    return s_wifi_ssid[0] ? s_wifi_ssid : JARVIS_WIFI_SSID;
}
void settingsSetWifiSSID(const char * v) { strncpy(s_wifi_ssid, v, sizeof(s_wifi_ssid) - 1); s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0'; }

const char * settingsGetWifiPassword()
{
    return s_wifi_password[0] ? s_wifi_password : JARVIS_WIFI_PASSWORD;
}
void settingsSetWifiPassword(const char * v) { strncpy(s_wifi_password, v, sizeof(s_wifi_password) - 1); s_wifi_password[sizeof(s_wifi_password) - 1] = '\0'; }

const char * settingsGetBackendHost()
{
    return s_backend_host[0] ? s_backend_host : JARVIS_BACKEND_HOST;
}
void settingsSetBackendHost(const char * v) { strncpy(s_backend_host, v, sizeof(s_backend_host) - 1); s_backend_host[sizeof(s_backend_host) - 1] = '\0'; }

int settingsGetBackendPort()
{
    return s_backend_port > 0 ? s_backend_port : JARVIS_BACKEND_PORT;
}
void settingsSetBackendPort(int v) { s_backend_port = v; }

const char * settingsGetMqttHost()
{
    return s_mqtt_host[0] ? s_mqtt_host : JARVIS_MQTT_HOST;
}
void settingsSetMqttHost(const char * v) { strncpy(s_mqtt_host, v, sizeof(s_mqtt_host) - 1); s_mqtt_host[sizeof(s_mqtt_host) - 1] = '\0'; }

int settingsGetMqttPort()
{
    return s_mqtt_port > 0 ? s_mqtt_port : JARVIS_MQTT_PORT;
}

void settingsSetMqttPort(int v) { s_mqtt_port = v; }

bool settingsGetAmbientVadEnabled() { return s_ambient_vad; }

void settingsSetAmbientVadEnabled(bool v) { s_ambient_vad = v; }

bool settingsGetPowerSavingEnabled() { return s_power_saving; }

void settingsSetPowerSavingEnabled(bool v) { s_power_saving = v; }

int  settingsGetScreenOffTimeout() { return s_screen_off; }

void settingsSetScreenOffTimeout(int v) { s_screen_off = v; }

bool settingsGetScreenLockEnabled() { return s_screen_lock; }

void settingsSetScreenLockEnabled(bool v) { s_screen_lock = v; }

void settingsGetAlarm(int * hour, int * minute, bool * enabled)
{
    if (hour) *hour = s_alarm_hour;
    if (minute) *minute = s_alarm_minute;
    if (enabled) *enabled = s_alarm_enabled;
}

void settingsSetAlarm(int hour, int minute, bool enabled)
{
    s_alarm_hour = hour;
    s_alarm_minute = minute;
    s_alarm_enabled = enabled;
}
