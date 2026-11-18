// NOTE:
//   The AXS5106L capacitive touch controller (I2C), single touch.
//   Requires: Wire, TOUCH_SDA / TOUCH_SCL / TOUCH_RST / TOUCH_INT as build flags.
//
//   Input::TouchPanel touch;
//   touch.begin(172, 320);               // in setup()
//   Input::TouchPoint point;
//   if (touch.poll(point)) { ... }       // in loop(), true on every touch report

//
//   Chapters:
//     READ THE TOUCH PANEL   The TouchPoint struct and the TouchPanel class.
//

#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace Input
{
    // ===========================================================================
    // ========================   READ THE TOUCH PANEL   =========================
    // ===========================================================================
    //
    //   Lifecycle   begin, I2C bus, reset and the interrupt.
    //   Polling     poll, reads one report after an interrupt.
    //

    /** @brief One touch report in display coordinates (portrait, origin top left). */
    struct TouchPoint
    {
        uint16_t x      = 0;
        uint16_t y      = 0;
        bool isNewTouch = false; // true on the first report after the finger came down
    };

    class TouchPanel
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, I2C bus, reset and the interrupt.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Resets the controller and listens on its interrupt line.
         * @param width  Display width in pixels, e.g. 172.
         * @param height Display height in pixels, e.g. 320.
         */
        void begin(uint16_t width, uint16_t height);

    private:
        uint16_t _width  = 0;
        uint16_t _height = 0;

    // ---------------------------------------------------------------------------
    // Polling
    //   poll, reads one report after an interrupt.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Reads the pending touch report, if the controller signalled one.
         * @param point Filled with the position when the call returns true.
         * @return True when a finger is on the panel and a new report came in.
         */
        bool poll(TouchPoint &point);

    private:
        // Reports further apart than this count as a new touch, in case the release report is lost.
        static constexpr uint32_t NEW_TOUCH_GAP_MS = 150;

        static void IRAM_ATTR onInterrupt();
        bool readRegister(uint8_t reg, uint8_t *data, size_t length);

        static volatile bool _hasReport; // set by the interrupt, cleared in poll
        bool _isTouching       = false;
        uint32_t _lastReportMs = 0;
    };
}
