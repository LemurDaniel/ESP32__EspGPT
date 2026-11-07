// NOTE:
//   Status page on the 1.47" ST7789 (172x320): on / off, timer and a live preview of the candle LEDs.
//   Requires: LCD_* pins and LED_COUNT as build flags, Adafruit ST7789 and GFX.
//
//   Display::StatusDisplay screen;
//   screen.begin();
//   screen.update(leds.isOn(), leds.revision(), leds.pixels(), timer.statusLine());   // in loop()

//
//   Chapters:
//     SHOW THE LED STATUS   The StatusDisplay class.
//

#pragma once

#include <Arduino.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <FastLED.h>

namespace Display
{
    // ===========================================================================
    // =========================   SHOW THE LED STATUS   =========================
    // ===========================================================================
    //
    //   Lifecycle   begin and update.
    //   Page        Static parts, redrawn only when the state changed.
    //   Timer       The timer line, redrawn only when its text changed.
    //   Preview     One dot per LED, only changed dots are redrawn.
    //

    class StatusDisplay
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin and update.
    // ---------------------------------------------------------------------------
    public:
        StatusDisplay();

        /** @brief Initializes the panel and switches the backlight on. */
        void begin();

        /**
         * @brief Redraws what changed since the last call.
         * @param isOn     True while the flame is burning.
         * @param revision LedController::revision() of that state.
         * @param pixels   LedController::pixels(), LED_COUNT entries.
         * @param timerLine CandleTimer::statusLine() (e.g. `"Timer 8h  aus in 3:12"`).
         */
        void update(bool isOn, uint32_t revision, const CRGB *pixels, const String &timerLine);

    private:
        static constexpr uint16_t WIDTH         = 172;
        static constexpr uint16_t HEIGHT        = 320;
        static constexpr uint8_t BACKLIGHT_DUTY = 160; // of 255, full brightness is not needed indoors

        Adafruit_ST7789 _tft;

    // ---------------------------------------------------------------------------
    // Page
    //   Static parts, redrawn only when the state changed.
    // ---------------------------------------------------------------------------
    private:
        void drawPage(bool isOn);

        bool _hasDrawn          = false;
        uint32_t _drawnRevision = 0;

    // ---------------------------------------------------------------------------
    // Timer
    //   The timer line, redrawn only when its text changed.
    // ---------------------------------------------------------------------------
    private:
        void drawTimerLine(const String &timerLine);

        String _drawnTimerLine; // cleared by drawPage, forces a redraw

    // ---------------------------------------------------------------------------
    // Preview
    //   One dot per LED, only changed dots are redrawn.
    // ---------------------------------------------------------------------------
    private:
        static constexpr uint32_t PREVIEW_INTERVAL_MS = 40; // 25 fps keeps the SPI bus mostly idle

        void drawPreviewFrame(const CRGB *pixels);
        void drawDot(uint16_t index, uint16_t color);

        uint16_t _dotColors[LED_COUNT] = {};
        bool _isPreviewStale           = true; // set by drawPage, forces every dot to redraw
        uint32_t _lastPreviewMs        = 0;
    };
}
