// NOTE:
//   Owns the WS2812 strip: power, colour, brightness, the effects and the power limit.
//   The settings are persisted in NVS.
//   Requires: FastLED, LED_DATA_PIN / LED_COUNT / LED_BRIGHTNESS / LED_MAX_MILLIAMPS
//   as build flags. Only use it from the loop task, it has no locking.
//
//   Leds::StripController strip;
//   strip.begin();                           // in setup()
//   strip.update();                          // in loop(), renders one frame
//   strip.setMode(Leds::StripMode::COMET);   // from the touch UI

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
    //   Settings      Power, mode, hue, saturation, brightness and the revision.
    //   Rendering     update, draws one frame of the active effect.
    //   Persistence   Writes the settings to NVS a while after a change.
    //

    /** @brief The effects of the strip, the values are persisted and index the UI buttons. */
    enum class StripMode : uint8_t
    {
        SOLID   = 0, // one colour on all LEDs
        RAINBOW = 1, // moving rainbow, ignores hue and saturation
        BREATHE = 2, // the colour fading in and out
        COMET   = 3, // a dot with a fading tail running along the strip
        CANDLE  = 4, // flickering candle flame, ignores hue and saturation
    };

    static constexpr uint8_t STRIP_MODE_COUNT = 5;

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
    //   Power, mode, hue, saturation, brightness and the revision.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Turns the strip on or off. */
        void togglePower();

        /**
         * @brief Sets the strip to a given state, does nothing when it already is.
         * @param isOn True to light the strip.
         */
        void setPower(bool isOn);

        /** @brief Sets the effect, does nothing when it already is active. */
        void setMode(StripMode mode);

        /** @brief Sets the colour hue, 0 to 255 around the FastLED rainbow colour wheel. */
        void setHue(uint8_t hue);

        /** @brief Sets the colour saturation, 0 (white) to 255 (full colour). */
        void setSaturation(uint8_t saturation);

        /** @brief Sets the global brightness, 0 to 255. The power limit may scale it down further. */
        void setBrightness(uint8_t brightness);

        bool isOn() const { return _isOn; }
        StripMode mode() const { return _mode; }
        uint8_t hue() const { return _hue; }
        uint8_t saturation() const { return _saturation; }
        uint8_t brightness() const { return _brightness; }

        /** @brief Increments on every change, lets the UI redraw only when needed. */
        uint32_t revision() const { return _revision; }

    private:
        /** @brief Bumps the revision and schedules the delayed save. */
        void markChanged();

        bool _isOn          = true;
        StripMode _mode     = StripMode::RAINBOW;
        uint8_t _hue        = 0;
        uint8_t _saturation = 255;
        uint8_t _brightness = LED_BRIGHTNESS;
        uint32_t _revision  = 0;

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
        uint8_t _gustLevel     = 0; // current dip of a gust in CANDLE, decays every frame
        uint32_t _lastFrameMs  = 0;
        uint8_t _rainbowHue    = 0; // hue of the first LED in RAINBOW, advances every frame
        uint16_t _cometIndex   = 0; // head of the comet in COMET

    // ---------------------------------------------------------------------------
    // Persistence
    //   Writes the settings to NVS a while after a change.
    // ---------------------------------------------------------------------------
    private:
        // Delayed, so dragging a slider writes the flash only once.
        static constexpr uint32_t SAVE_DELAY_MS = 3000;

        void load();
        void save();

        Preferences _prefs;
        bool _isSavePending    = false;
        uint32_t _lastChangeMs = 0;
    };
}
