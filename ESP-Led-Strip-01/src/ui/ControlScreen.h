// NOTE:
//   The touch UI of the strip on the 172x320 portrait display: a power tile, the colour
//   bar to drag, an effect tile and a timer tile. A tap on a tile selects its next option.
//   Requires: a started Display::LcdPanel, Leds::StripController and Timers::DailyTimer.
//   Only use it from the loop task.
//
//   Ui::ControlScreen screen;
//   screen.begin(panel.gfx(), strip, timer);   // in setup(), after panel, strip and timer
//   screen.handleTouch(point);                 // in loop(), on every touch report
//   screen.update();                           // in loop(), redraws what changed

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
    //   Lifecycle   begin, clears the screen and draws the rows.
    //   Touch       handleTouch, taps on the tiles and drags on the colour bar.
    //   Drawing     update, redraws the rows whose values changed.
    //
    //   Layout, four rows of 72 px with 8 px gaps, top to bottom:
    //     Power    Tile: "AN" in the selected colour or "AUS" in grey.
    //     Colour   Bar: warm white, white, then the colour wheel. Drag to select.
    //     Effect   Tile: name of the effect.
    //     Timer    Tile: preset and the time until the next switch.
    //

    class ControlScreen
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, clears the screen and draws the rows.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Clears the screen and draws all rows.
         * @param gfx   Graphics context of the started display.
         * @param strip The strip the controls act on.
         * @param timer The timer behind the timer tile.
         */
        void begin(Arduino_GFX &gfx, Leds::StripController &strip, Timers::DailyTimer &timer);

    private:
        Arduino_GFX *_gfx             = nullptr;
        Leds::StripController *_strip = nullptr;
        Timers::DailyTimer *_timer    = nullptr;

    // ---------------------------------------------------------------------------
    // Touch
    //   handleTouch, taps on the tiles and drags on the colour bar.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Applies one touch report.
         * @param point Report from Input::TouchPanel::poll.
         */
        void handleTouch(const Input::TouchPoint &point);

    private:
        // The rows top to bottom, the value is the row index.
        enum class Row : uint8_t
        {
            POWER  = 0,
            COLOR  = 1,
            EFFECT = 2,
            TIMER  = 3,
        };

        // True while the finger that came down on the colour bar is still on the panel.
        // The drag keeps the bar, also when the finger drifts onto a tile.
        bool _isDraggingColor = false;

    // ---------------------------------------------------------------------------
    // Drawing
    //   update, redraws the rows whose values changed.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Redraws the rows that changed since the last call, at most 30 times a second. */
        void update();

    private:
        static constexpr uint32_t DRAW_INTERVAL_MS = 33;

        void drawPowerTile();
        void drawColorBar();
        void drawEffectTile();
        void drawTimerTile();
        void drawTile(Row row, uint16_t background, uint16_t foreground, const char *label, const char *value,
                      const char *detail);

        // What the screen currently shows
        bool _isFirstDraw            = true;
        bool _drawnIsOn              = false;
        Leds::StripMode _drawnMode   = Leds::StripMode::SOLID;
        uint8_t _drawnColorPosition  = 0;
        uint32_t _drawnRevision      = 0;
        uint32_t _drawnTimerRevision = 0;
        uint32_t _lastDrawMs         = 0;
    };
}
