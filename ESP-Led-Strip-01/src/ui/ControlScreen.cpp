// NOTE:
//   Implementation of ControlScreen.h.

//
//   Chapters:
//     HELPERS     Colour conversion and the readable text colour.
//     LIFECYCLE   begin.
//     TOUCH       handleTouch.
//     DRAWING     update and the draw functions of the tiles.
//

#include <ui/ControlScreen.h>

namespace Ui
{
    // Layout, see the chapter description in ControlScreen.h
    static constexpr int16_t TILES_Y     = 4;
    static constexpr int16_t TILE_X      = 6;
    static constexpr int16_t TILE_WIDTH  = 160;
    static constexpr int16_t TILE_HEIGHT = 72;
    static constexpr int16_t TILE_GAP    = 8;
    static constexpr int16_t TILE_RADIUS = 12;
    static constexpr int16_t TILE_COUNT  = 4;

    // Text positions inside a tile
    static constexpr int16_t TEXT_X       = 12;
    static constexpr int16_t LABEL_Y      = 10; // small caption
    static constexpr int16_t VALUE_Y      = 28; // the selected option, double size
    static constexpr int16_t DETAIL_Y     = 54; // optional small line below
    static constexpr uint8_t VALUE_SCALE  = 2;

    // RGB565
    static constexpr uint16_t COLOR_BACKGROUND = 0x0000;
    static constexpr uint16_t COLOR_PANEL      = 0x2124; // dark grey tile
    static constexpr uint16_t COLOR_WHITE      = 0xFFFF;
    static constexpr uint16_t COLOR_BLACK      = 0x0000;

    // Above this perceived brightness (0..255) black text reads better than white
    static constexpr uint8_t LUMA_DARK_TEXT = 140;

    // The display font has no umlauts. Indexed by Leds::StripMode.
    static constexpr const char *MODE_LABELS[Leds::STRIP_MODE_COUNT] = {"EINFARBIG", "KERZE", "ATMEN", "REGENBOGEN"};

    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   toRgb565      FastLED colour to the 16 bit colour of the display.
    //   textColorOn   Black or white, whichever reads better on a background.
    //

    static uint16_t toRgb565(const CRGB &color)
    {
        return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
    }

    static uint16_t textColorOn(const CRGB &background)
    {
        // Rec. 601 luma with integer weights: (77, 150, 29) / 256
        const uint8_t luma = (background.r * 77 + background.g * 150 + background.b * 29) >> 8;
        return luma > LUMA_DARK_TEXT ? COLOR_BLACK : COLOR_WHITE;
    }

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================

    void ControlScreen::begin(Arduino_GFX &gfx, Leds::StripController &strip, Timers::DailyTimer &timer)
    {
        _gfx   = &gfx;
        _strip = &strip;
        _timer = &timer;

        _gfx->fillScreen(COLOR_BACKGROUND);
        update();
    }

    // ===========================================================================
    // ================================   TOUCH   ================================
    // ===========================================================================

    void ControlScreen::handleTouch(const Input::TouchPoint &point)
    {
        // Only the first report of a touch is a tap, a resting finger would keep cycling
        if (!point.isNewTouch)
        {
            return;
        }

        // The gap below a tile and the margins count to the nearest tile
        const int16_t row = constrain((static_cast<int16_t>(point.y) - TILES_Y) / (TILE_HEIGHT + TILE_GAP), 0,
                                      TILE_COUNT - 1);
        switch (static_cast<Tile>(row))
        {
        case Tile::POWER:  _strip->togglePower(); break;
        case Tile::COLOR:  _strip->cycleColor();  break;
        case Tile::EFFECT: _strip->cycleMode();   break;
        case Tile::TIMER:  _timer->cycleStep();   break;
        }
    }

    // ===========================================================================
    // ===============================   DRAWING   ===============================
    // ===========================================================================
    //
    //   update           Redraws the strip tiles on a strip change, the timer tile on
    //                    a timer change. Changes only come from taps and once a minute
    //                    from the timer, so whole tiles are redrawn.
    //   drawPowerTile    "AN" in the selected colour or "AUS" in grey.
    //   drawColorTile    Name of the preset on a tile of that colour.
    //   drawEffectTile   Name of the effect.
    //   drawTimerTile    Preset and the time until the next switch.
    //   drawTile         One tile: caption, value and an optional detail line.
    //

    void ControlScreen::update()
    {
        const bool isStripChanged = _isFirstDraw || _strip->revision() != _drawnRevision;
        const bool isTimerChanged = _isFirstDraw || _timer->revision() != _drawnTimerRevision;
        if (!isStripChanged && !isTimerChanged)
        {
            return;
        }
        _drawnRevision      = _strip->revision();
        _drawnTimerRevision = _timer->revision();
        _isFirstDraw        = false;

        if (isStripChanged)
        {
            drawPowerTile();
            drawColorTile();
            drawEffectTile();
        }
        if (isTimerChanged)
        {
            drawTimerTile();
        }
    }

    void ControlScreen::drawPowerTile()
    {
        if (!_strip->isOn())
        {
            drawTile(Tile::POWER, COLOR_PANEL, COLOR_WHITE, "LICHT", "AUS", nullptr);
            return;
        }
        const CRGB &color = _strip->colorPreset().color;
        drawTile(Tile::POWER, toRgb565(color), textColorOn(color), "LICHT", "AN", nullptr);
    }

    void ControlScreen::drawColorTile()
    {
        const Leds::ColorPreset &preset = _strip->colorPreset();
        drawTile(Tile::COLOR, toRgb565(preset.color), textColorOn(preset.color), "FARBE", preset.label, nullptr);
    }

    void ControlScreen::drawEffectTile()
    {
        drawTile(Tile::EFFECT, COLOR_PANEL, COLOR_WHITE, "EFFEKT", MODE_LABELS[static_cast<uint8_t>(_strip->mode())],
                 nullptr);
    }

    void ControlScreen::drawTimerTile()
    {
        if (0 == _timer->hours())
        {
            drawTile(Tile::TIMER, COLOR_PANEL, COLOR_WHITE, "TIMER", "AUS", nullptr);
            return;
        }

        char value[8];
        char status[16];
        snprintf(value, sizeof(value), "%u H", _timer->hours());
        _timer->statusLine(status, sizeof(status));
        drawTile(Tile::TIMER, COLOR_PANEL, COLOR_WHITE, "TIMER", value, status);
    }

    void ControlScreen::drawTile(Tile tile, uint16_t background, uint16_t foreground, const char *label,
                                 const char *value, const char *detail)
    {
        const int16_t y = TILES_Y + static_cast<int16_t>(tile) * (TILE_HEIGHT + TILE_GAP);

        _gfx->fillRoundRect(TILE_X, y, TILE_WIDTH, TILE_HEIGHT, TILE_RADIUS, background);
        _gfx->setTextColor(foreground);

        _gfx->setTextSize(1);
        _gfx->setCursor(TILE_X + TEXT_X, y + LABEL_Y);
        _gfx->print(label);

        _gfx->setTextSize(VALUE_SCALE);
        _gfx->setCursor(TILE_X + TEXT_X, y + VALUE_Y);
        _gfx->print(value);

        if (nullptr != detail)
        {
            _gfx->setTextSize(1);
            _gfx->setCursor(TILE_X + TEXT_X, y + DETAIL_Y);
            _gfx->print(detail);
        }
    }
}
