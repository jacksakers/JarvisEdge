// Project  : House Phone
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
#define JARVIS_BACKEND_PORT    8010
#endif

// Mosquitto broker — defaults to the same host as the backend.
#ifndef JARVIS_MQTT_HOST
#define JARVIS_MQTT_HOST       JARVIS_BACKEND_HOST
#endif
#ifndef JARVIS_MQTT_PORT
#define JARVIS_MQTT_PORT       1883
#endif

// Topic the backend publishes UI updates to (docs/sdd.txt Card 4; must
// match backend/config.yaml's mqtt.topic_feed).
#define JARVIS_MQTT_TOPIC_FEED   "jarvis/ui/feed"
