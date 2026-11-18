// NOTE:
//   The touch UI of the strip: four full-width tiles (power, colour, effect, timer)
//   on the 172x320 portrait display. Every tap selects the next option of its tile.
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
    //   Lifecycle   begin, clears the screen and draws the tiles.
    //   Touch       handleTouch, a tap acts on the tile below the finger.
    //   Drawing     update, redraws the tiles whose values changed.
    //
    //   Layout, four tiles of 72 px with 8 px gaps, top to bottom:
    //     Power    "AN" in the selected colour or "AUS" in grey.
    //     Colour   Name of the preset on a tile of that colour.
    //     Effect   Name of the effect.
    //     Timer    Preset and the time until the next switch.
    //

    class ControlScreen
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, clears the screen and draws the tiles.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Clears the screen and draws all tiles.
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
    //   handleTouch, a tap acts on the tile below the finger.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Applies one touch report, only the first report of a touch counts as a tap.
         * @param point Report from Input::TouchPanel::poll.
         */
        void handleTouch(const Input::TouchPoint &point);

    // ---------------------------------------------------------------------------
    // Drawing
    //   update, redraws the tiles whose values changed.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Redraws the tiles that changed since the last call. */
        void update();

    private:
        // The tiles top to bottom, the value is the row.
        enum class Tile : uint8_t
        {
            POWER  = 0,
            COLOR  = 1,
            EFFECT = 2,
            TIMER  = 3,
        };

        void drawPowerTile();
        void drawColorTile();
        void drawEffectTile();
        void drawTimerTile();
        void drawTile(Tile tile, uint16_t background, uint16_t foreground, const char *label, const char *value,
                      const char *detail);

        bool _isFirstDraw            = true;
        uint32_t _drawnRevision      = 0;
        uint32_t _drawnTimerRevision = 0;
    };
}
