# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash Commands

```bash
# Build
pio run -e ESP32-S3-dev

# Flash firmware
pio run -e ESP32-S3-dev --target upload

# Upload web UI (LittleFS filesystem image)
pio run -e ESP32-S3-dev --target uploadfs

# Serial monitor
pio device monitor --baud 115200
```

Available board environments: `ESP32-S3-dev`, `ESP32-WROOM-32`, `freenove_esp32_wrover`.

There are no automated tests in this project.

## Architecture

EspGPT is a PlatformIO/Arduino project that turns an ESP32 into a browser-accessible AI chat terminal backed by Azure AI Foundry.

**Request flow:**
```
Browser  →  POST /chat  →  routes_gpt::Router  →  AzureFoundryClient  →  Azure OpenAI API
```

### Framework: ESP32 MiniWebServer

The project depends on [ESP32__MiniWebServer-Framework](https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework) (pulled via `lib_deps` in `platformio.ini`). It provides:

- `EspWeb::MiniServer` — starts the HTTP server, registers routers, and serves static files from LittleFS.
- `EspWeb::Router` — base class for route groups; subclass it and register handlers in the constructor.
- `EspWeb::Request` / `EspWeb::Response` — handler arguments; use `request.body.json()` for JSON bodies and `writeJsonFile()` (framework utility) to persist to LittleFS.

`main.cpp` is intentionally minimal: it registers the `routes_gpt::Router`, sets the index page and a named admin link, then calls `Server.start(80)`. The `loop()` is an idle spin.

### Azure AI Foundry Client (`src/foundryClient.h`)

`AzureFoundryClient` wraps `WiFiClientSecure` + `HTTPClient` and posts to `/openai/v1/chat/completions`. Key points:

- `begin(url, key)` stores credentials and calls `_client.setInsecure()` (no TLS cert validation — for production replace with `setCACert()`).
- `buildJson()` constructs the request body with a system prompt and user message.
- The response is parsed with ArduinoJson's **filter API** to extract only `choices[0].message.content`, minimising heap usage on the ESP32.
- On error, `status()` returns the HTTP code and `error()` returns the raw Azure error JSON or an `HTTPClient` error string.

### GPT Settings persistence

Settings (Base URL, API key, model name) are POSTed to `/admin/settings/gpt`, written to `/gpt.settings.json` on LittleFS by `routes_gpt::Router::set_GptSettings`, and read back on each request. The settings page lives at `/admin/settings` (`data/web/settings.html`).

### Web UI (`data/web/`)

Static files served from LittleFS. `index.html` is a self-contained dark-themed chat SPA that POSTs `{ msg: "..." }` to `/chat` and renders the plain-text reply. Must be uploaded separately with `uploadfs` after any HTML/CSS changes.
