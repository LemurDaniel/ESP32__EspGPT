# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this project.

## Build & Flash Commands

```powershell
pio run -e waveshare-c6-touch-lcd147                     # Build
pio run -e waveshare-c6-touch-lcd147 --target upload     # Flash firmware
pio device monitor --baud 115200                         # Serial monitor
```

Single environment: `waveshare-c6-touch-lcd147`. No automated tests.

- Run PlatformIO from **PowerShell**, not Git Bash: the pioarduino tool installer (`idf_tools.py`) aborts with "MSys/Mingw is not supported".
- Windows long paths must be enabled, the C6 framework libs exceed 260 characters.
- `lib_ignore = SD` must stay: FastLED pulls the core's SD library in, which does not build on the C6.

## Hardware constraints

- Board: Waveshare ESP32-C6-**Touch**-LCD-1.47 with ESP32-C6FH8 (RISC-V **single core**, 160 MHz, 8 MB flash). Needs the **pioarduino** platform (Arduino core 3.x).
- This is **not** the board of `../ESP-Led-deko` (non-touch, ST7789): the pinout differs completely. Do not copy pins from there.
- Pins come from `build_flags` in `platformio.ini`. Never hard-code pins in source.
- Taken by the board: 1, 2 (LCD + TF SPI), 3, 4 (TF), 14, 15, 22, 23 (LCD), 18, 19, 20, 21 (touch + IMU), 5, 6 (IMU INT), 8 (BOOT), 0 (battery ADC), 12, 13 (USB), 16, 17 (UART0). Only **GPIO7** is free on the header.
- Strip: BTF-LIGHTING WS2812 ECO, 144 LEDs, GRB, DIN on GPIO7, fed from VBUS (USB). Keep `FastLED.setMaxPowerInVoltsAndMilliamps` in place, full white would draw ~8.6 A.
- Display: JD9853. Arduino_GFX has no class for it, `LcdPanel` uses `Arduino_ST7789` (column offset 34) plus the vendor register init from the Waveshare demo zip (`Arduino/examples/06_lvgl_brightness`). The code printed in the Waveshare wiki uses pins of a different board, do not trust it.
- Touch: AXS5106L on I2C `0x63`, report = 14 bytes from register `0x01`, x axis mirrored against the display. Protocol from the demo's `esp_lcd_touch_axs5106l` library.
- GFX Library for Arduino must be 1.6 or newer: 1.5.9 (the version the demo ships) does not compile against this Arduino core.
- Not verified: I2C address of the IMU, onboard RGB LED.

## Architecture

```
loop() ─► TouchPanel::poll()          one report per INT -> ControlScreen::handleTouch()
       ├► DailyTimer::poll()          TURN_ON / TURN_OFF at the switches of the daily cycle -> setPower()
       ├► StripController::update()   effect frame every 20 ms, delayed NVS save
       └► ControlScreen::update()     redraws only parts whose values changed, max 30 fps
```

- Everything runs in the loop task, so no class has locking. Only `TouchPanel::onInterrupt` runs in an ISR and just sets a flag.
- `ControlScreen` never draws from `handleTouch`: touches change `StripController`, its `revision()` triggers the redraw.
- The UI is deliberately minimal (user decision): tiles for power, effect and timer, one bar for the colour. No sliders for brightness or saturation, brightness is the fixed `LED_BRIGHTNESS`.
- Tiles react only to `TouchPoint::isNewTouch`, the colour bar to every report of a drag.
- `StripController::colorAt` is the one mapping from bar position to colour (warm white, white, colour wheel). The UI draws the bar with it, so bar and strip cannot drift apart.
- `DailyTimer` has no real-time clock: the cycle starts when the preset is set or at boot. It uses `esp_timer_get_time()` (64 bit) because `millis()` wraps after 49 days. Events fire only at phase switches, so a manual toggle holds until the next switch. Preset in NVS namespace `timer`.
- The effect tile cycles through `StripMode` in enum order. A new mode needs a label in `MODE_LABELS` (ControlScreen.cpp).
- Settings (power, effect, colour position) are persisted in NVS (`Preferences`, namespace `strip`) 3 s after the last change.

## Code style

Follows the user's code-style skill (chapter/section banners, Allman, 4 spaces, Doxygen on members).
