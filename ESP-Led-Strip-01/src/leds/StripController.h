// NOTE:
//   Owns the WS2812 strip: power, colour, the effects and the power limit.
//   The settings are persisted in NVS.
//   Requires: FastLED, LED_DATA_PIN / LED_COUNT / LED_BRIGHTNESS / LED_MAX_MILLIAMPS
//   as build flags. Only use it from the loop task, it has no locking.
//
//   Leds::StripController strip;
//   strip.begin();        // in setup()
//   strip.update();       // in loop(), renders one frame
//   strip.setColorPosition(120);   // from the touch UI, position on the colour bar
//   strip.cycleMode();             // from the touch UI, next effect

//
//   Chapters:
//     CONTROL THE STRIP   The StripMode enum and the StripController class.
//

#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <FastLED.h>

namespace Leds
{
    // ===========================================================================
    // ==========================   CONTROL THE STRIP   ==========================
    // ===========================================================================
    //
    //   Lifecycle     begin, loads the settings and registers the strip.
    //   Settings      Power, effect, colour and the revision.
    //   Rendering     update, draws one frame of the active effect.
    //   Persistence   Writes the settings to NVS a while after a change.
    //

    /** @brief The effects of the strip in the order the UI cycles them, the values are persisted. */
    enum class StripMode : uint8_t
    {
        SOLID   = 0, // the selected colour on all LEDs
        CANDLE  = 1, // flickering candle flame, ignores the selected colour
        BREATHE = 2, // the selected colour fading in and out
        RAINBOW = 3, // moving rainbow, ignores the selected colour
    };

    static constexpr uint8_t STRIP_MODE_COUNT = 4;


    class StripController
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, loads the settings and registers the strip.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Loads the persisted settings and sets up FastLED with the power limit. */
        void begin();

    // ---------------------------------------------------------------------------
    // Settings
    //   Power, effect, colour and the revision.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Turns the strip on or off. */
        void togglePower();

        /**
         * @brief Sets the strip to a given state, does nothing when it already is.
         * @param isOn True to light the strip.
         */
        void setPower(bool isOn);

        /** @brief Selects the next effect, after the last one the first again. */
        void cycleMode();

        /**
         * @brief Selects the colour by its position on the colour bar.
         * @param position 0 to 255, see colorAt.
         */
        void setColorPosition(uint8_t position);

        /**
         * @brief The colour at a position of the colour bar, the one mapping UI and strip share.
         * @param position 0..15 warm white, 16..31 white, 32..255 once around the colour wheel.
         */
        static CRGB colorAt(uint8_t position);

        bool isOn() const { return _isOn; }
        StripMode mode() const { return _mode; }
        uint8_t colorPosition() const { return _colorPosition; }

        /** @brief The selected colour. */
        CRGB color() const { return colorAt(_colorPosition); }

        /** @brief Increments on every change, lets the UI redraw only when needed. */
        uint32_t revision() const { return _revision; }

    private:
        /** @brief Bumps the revision and schedules the delayed save. */
        void markChanged();

        bool _isOn             = true;
        StripMode _mode        = StripMode::SOLID;
        uint8_t _colorPosition = 0; // warm white
        uint32_t _revision     = 0;

    // ---------------------------------------------------------------------------
    // Rendering
    //   update, draws one frame of the active effect.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Renders one frame when the frame interval has passed, also runs the delayed save. */
        void update();

    private:
        static constexpr uint32_t FRAME_INTERVAL_MS = 20; // 50 fps

        void renderCandle(uint32_t now);

        CRGB _strip[LED_COUNT];
        uint32_t _lastFrameMs = 0;
        uint8_t _rainbowHue   = 0; // hue of the first LED in RAINBOW, advances every frame
        uint8_t _gustLevel    = 0; // current dip of a gust in CANDLE, decays every frame

    // ---------------------------------------------------------------------------
    // Persistence
    //   Writes the settings to NVS a while after a change.
    // ---------------------------------------------------------------------------
    private:
        // Delayed, so dragging the colour bar writes the flash only once.
        static constexpr uint32_t SAVE_DELAY_MS = 3000;

        void load();
        void save();

        Preferences _prefs;
        bool _isSavePending    = false;
        uint32_t _lastChangeMs = 0;
    };
}
