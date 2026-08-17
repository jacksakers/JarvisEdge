// Project  : Jarvis Edge Node
// File     : network_config.h
// Purpose  : Compile-time WiFi/backend/MQTT settings (Phase 4)
// Depends  : (none)
//
// Override any of these via platformio.ini build_flags, e.g.:
//   -DJARVIS_WIFI_SSID='"My Network"'
//   -DJARVIS_WIFI_PASSWORD='"secret"'
//   -DJARVIS_BACKEND_HOST='"192.168.1.50"'

#pragma once

#ifndef JARVIS_WIFI_SSID
#define JARVIS_WIFI_SSID       ""
#define JARVIS_WIFI_PASSWORD   ""
#endif

// Home server running the Phase 3 FastAPI backend (docs/sdd.txt 2.2).
#ifndef JARVIS_BACKEND_HOST
#define JARVIS_BACKEND_HOST    "192.168.1.88"
#endif
#ifndef JARVIS_BACKEND_PORT
#define JARVIS_BACKEND_PORT    8000
#endif

// Mosquitto broker — defaults to the same host as the backend.
#ifndef JARVIS_MQTT_HOST
#define JARVIS_MQTT_HOST       JARVIS_BACKEND_HOST
#endif
#ifndef JARVIS_MQTT_PORT
#define JARVIS_MQTT_PORT       1883
#endif

// Topics the backend publishes UI updates to (docs/sdd.txt 4.2/4.3;
// must match backend/config.yaml's mqtt.topic_feed / mqtt.topic_focus).
#define JARVIS_MQTT_TOPIC_FEED   "jarvis/ui/feed"
#define JARVIS_MQTT_TOPIC_FOCUS  "jarvis/ui/focus"
