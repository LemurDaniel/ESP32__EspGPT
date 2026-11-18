// NOTE:
//   The touch UI of the strip: power pill, three sliders (hue, saturation, brightness),
//   five effect buttons and the timer button on the 172x320 portrait display.
//   Requires: a started Display::LcdPanel, Leds::StripController and Timers::DailyTimer.
//   Only use it from the loop task.
//
//   Ui::ControlScreen screen;
//   screen.begin(panel.gfx(), strip, timer);   // in setup(), after panel, strip and timer
//   screen.handleTouch(point);           // in loop(), on every touch report
//   screen.update();                     // in loop(), redraws what changed

//
//   Chapters:
//     SHOW THE CONTROLS   The ControlScreen class.
//

#pragma once

#include <Arduino.h>

#include <Arduino_GFX_Library.h>

#include <input/TouchPanel.h>
#include <leds/StripController.h>
#include <timer/DailyTimer.h>

namespace Ui
{
    // ===========================================================================
    // ==========================   SHOW THE CONTROLS   ==========================
    // ===========================================================================
    //
    //   Lifecycle   begin, draws the static parts.
    //   Touch       handleTouch, maps a touch report to a strip setting.
    //   Drawing     update, redraws only the parts whose values changed.
    //
    //   Layout, top to bottom:
    //     0..39      Header: title and the power pill, a tap toggles the power.
    //     40..201    Three slider rows of 54 px: label, then a 28 px gradient bar.
    //     208..319   Buttons in a 2x3 grid: five effects, then the timer.
    //

    class ControlScreen
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, draws the static parts.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Draws title and labels, the values follow with the first update().
         * @param gfx   Graphics context of the started display.
         * @param strip The strip the controls act on.
         * @param timer The timer behind the timer button.
         */
        void begin(Arduino_GFX &gfx, Leds::StripController &strip, Timers::DailyTimer &timer);

    private:
        Arduino_GFX *_gfx             = nullptr;
        Leds::StripController *_strip = nullptr;
        Timers::DailyTimer *_timer    = nullptr;

    // ---------------------------------------------------------------------------
    // Touch
    //   handleTouch, maps a touch report to a strip setting.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Applies one touch report: taps on header and buttons, drags on the sliders.
         * @param point Report from Input::TouchPanel::poll.
         */
        void handleTouch(const Input::TouchPoint &point);

    private:
        enum class Slider : uint8_t
        {
            HUE        = 0,
            SATURATION = 1,
            BRIGHTNESS = 2,
        };

        static constexpr int8_t NO_SLIDER = -1;

        void applySlider(Slider slider, uint16_t x);
        void pressButton(const Input::TouchPoint &point);

        // The slider the finger came down on. A drag keeps it, also when it drifts into another row.
        int8_t _activeSlider = NO_SLIDER;

    // ---------------------------------------------------------------------------
    // Drawing
    //   update, redraws only the parts whose values changed.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Redraws the parts that changed since the last call, at most 30 times a second. */
        void update();

    private:
        static constexpr uint32_t DRAW_INTERVAL_MS = 33;

        /** @brief The strip values the screen currently shows. */
        struct DrawnState
        {
            bool isOn             = false;
            Leds::StripMode mode  = Leds::StripMode::SOLID;
            uint8_t hue           = 0;
            uint8_t saturation    = 0;
            uint8_t brightness    = 0;
        };

        void drawPowerPill();
        void drawSlider(Slider slider);
        void drawModeButtons();
        void drawTimerButton();
        void drawButton(uint8_t index, bool isActive, const char *line1, const char *line2);
        uint16_t accentColor() const;

        DrawnState _drawn;
        bool _isFirstDraw            = true;
        uint32_t _drawnRevision      = 0;
        uint32_t _drawnTimerRevision = 0;
        uint32_t _lastDrawMs         = 0;
    };
}
