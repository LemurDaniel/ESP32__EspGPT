// NOTE:
//   Owns the WS2812B strip and the onboard LED: candle flame effect, power limit and the
//   persisted on / off state.
//   Requires: FastLED, LED_DATA_PIN / LED_COUNT / LED_MAX_MILLIAMPS / LED_BRIGHTNESS /
//   ONBOARD_LED_PIN as build flags. Only use it from the loop task, it has no locking.
//
//   Leds::LedController leds;
//   leds.begin();         // in setup()
//   leds.update();        // in loop(), renders one frame
//   leds.togglePower();   // on button press

//
//   Chapters:
//     CONTROL THE LEDS   The LedController class.
//

#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <FastLED.h>

namespace Leds
{
    // ===========================================================================
    // ==========================   CONTROL THE LEDS   ===========================
    // ===========================================================================
    //
    //   Lifecycle     begin, loads the state and registers both LED outputs.
    //   Power         togglePower, setPower, isOn and revision.
    //   Rendering     update and pixels, draws one frame of the candle flame.
    //   Persistence   Writes the on / off state to NVS a while after a change.
    //

    class LedController
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, loads the state and registers both LED outputs.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Loads the persisted state and sets up FastLED with the power limit. */
        void begin();

    // ---------------------------------------------------------------------------
    // Power
    //   togglePower, setPower, isOn and revision.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Turns the flame on or off and schedules a save. */
        void togglePower();

        /**
         * @brief Sets the flame to a given state, does nothing when it already is.
         * @param isOn True to light the flame.
         */
        void setPower(bool isOn);

        /** @brief True while the flame is burning. */
        bool isOn() const;

        /** @brief Increments on every change, lets the display redraw only when needed. */
        uint32_t revision() const;

    private:
        bool _isOn         = true;
        uint32_t _revision = 0;

    // ---------------------------------------------------------------------------
    // Rendering
    //   update and pixels, draws one frame of the candle flame.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Renders one frame (call from loop()), also runs the delayed save. */
        void update();

        /**
         * @brief The last rendered frame, before brightness and power limit are applied.
         * @return LED_COUNT pixels, all black while the flame is off.
         */
        const CRGB *pixels() const;

    private:
        static constexpr uint32_t FRAME_INTERVAL_MS = 20; // 50 fps

        void renderCandle(uint32_t now);

        CRGB _strip[LED_COUNT];
        CRGB _onboard[1];
        uint32_t _lastFrameMs = 0;
        uint8_t _gustLevel    = 0; // current dip of a gust, decays every frame

    // ---------------------------------------------------------------------------
    // Persistence
    //   Writes the on / off state to NVS a while after a change.
    // ---------------------------------------------------------------------------
    private:
        // Delayed, so toggling quickly several times writes the flash only once.
        static constexpr uint32_t SAVE_DELAY_MS = 3000;

        void load();
        void save();

        Preferences _prefs;
        bool _isSavePending    = false;
        uint32_t _lastChangeMs = 0;
    };
}
