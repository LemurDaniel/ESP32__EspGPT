# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this project.

## Build & Flash Commands

```powershell
pio run -e waveshare-c6-lcd147                     # Build
pio run -e waveshare-c6-lcd147 --target upload     # Flash firmware
pio device monitor --baud 115200                   # Serial monitor
```

Single environment: `waveshare-c6-lcd147`. No automated tests.

- Run PlatformIO from **PowerShell**, not Git Bash: the pioarduino tool installer (`idf_tools.py`) aborts with "MSys/Mingw is not supported" and leaves the RISC-V toolchain empty.
- Windows long paths must be enabled, the C6 framework libs exceed 260 characters.

## Hardware constraints

- Board: Waveshare ESP32-C6-LCD-1.47 with ESP32-C6FH8 (RISC-V **single core**, 160 MHz, 8 MB flash). Needs the **pioarduino** platform (Arduino core 3.x).
- Pins come from `build_flags` in `platformio.ini` (`LCD_*`, `ONBOARD_LED_PIN`, `BOOT_BUTTON_PIN`, `LED_DATA_PIN`, `LED_COUNT`, `LED_BRIGHTNESS`, `LED_MAX_MILLIAMPS`). Never hard-code pins in source.
- Avoid on the C6: 4, 5, 8, 9, 15 (strapping), 12, 13 (USB), 6, 7, 14, 15, 21, 22 (LCD), 4, 5 (SD card).
- LEDs: Freenove 8 RGB LED module (WS2812B) on GPIO2, USB powered. Keep `FastLED.setMaxPowerInVoltsAndMilliamps` in place.
- The display has **no touch**. Only input is the BOOT button.

## Architecture

```
loop() ─► BootButton::poll()       any press = LedController::togglePower()
       ├► LedController::update()  candle flame frame every 20 ms, delayed NVS save
       └► StatusDisplay::update()  page on revision change, ring preview of pixels() at 25 fps
```

- Everything runs in the loop task, so `LedController` has no locking.
- The candle flame samples a moving 2D noise field per LED (`inoise8`) for smooth flicker, plus rare random "gusts" that dim all LEDs and decay. Bright = yellow, dim = deep orange.
- The on / off state is persisted in NVS (`Preferences`, namespace `leds`) 3 s after the last change.
- No WiFi and no web server. An earlier version used ESP32__MiniWebServer-Framework, which crashed on the C6: its constructor mounts LittleFS (static-init order) and it pins worker `i` to core `i % 2`.

## Code style

Follows the user's code-style skill (chapter/section banners, Allman, 4 spaces, Doxygen on members).
