// NOTE:
//   Template for include/secrets.h, which holds the WiFi and admin credentials.
//   Copy this file to include/secrets.h and fill in your values. secrets.h is in
//   .gitignore and never committed.
//
//   Copy-Item include/secrets.example.h include/secrets.h

#pragma once

// WiFi the device joins on boot. Left as placeholder, the device opens its setup
// access point "ESP32_MiniWebServer" instead.
#define WIFI_SSID     "<SSID>"
#define WIFI_PASSWORD "<WIFI_PASSWORD>"

// Login of the MiniServer admin dashboard at /admin
#define ADMIN_USER     "admin"
#define ADMIN_PASSWORD "<ADMIN_PASSWORD>"
