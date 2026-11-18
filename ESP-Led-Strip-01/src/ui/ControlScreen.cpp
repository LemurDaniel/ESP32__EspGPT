// NOTE:
//   Implementation of ControlScreen.h.

//
//   Chapters:
//     HELPERS     Colour conversion, text colour and the colour bar mapping.
//     LIFECYCLE   begin.
//     TOUCH       handleTouch.
//     DRAWING     update and the draw functions of the rows.
//

#include <ui/ControlScreen.h>

namespace Ui
{
    // Layout, see the chapter description in ControlScreen.h
    static constexpr int16_t ROWS_Y      = 4;
    static constexpr int16_t ROW_X       = 6;
    static constexpr int16_t ROW_WIDTH   = 160;
    static constexpr int16_t ROW_HEIGHT  = 72;
    static constexpr int16_t ROW_GAP     = 8;
    static constexpr int16_t ROW_COUNT   = 4;
    static constexpr int16_t TILE_RADIUS = 12;

    // Text positions inside a row
    static constexpr int16_t TEXT_X      = 12;
    static constexpr int16_t LABEL_Y     = 10; // small caption
    static constexpr int16_t VALUE_Y     = 28; // the selected option, double size
    static constexpr int16_t DETAIL_Y    = 54; // optional small line below
    static constexpr uint8_t VALUE_SCALE = 2;

    // Colour bar: caption on top, the bar fills the rest of the row
    static constexpr int16_t BAR_Y          = 24; // inside the row
    static constexpr int16_t BAR_HEIGHT     = ROW_HEIGHT - BAR_Y;
    static constexpr int16_t MARKER_HALF    = 1; // white core of the marker, columns left and right
    static constexpr int16_t MARKER_OUTLINE = 3; // black outline around it

    // RGB565
    static constexpr uint16_t COLOR_BACKGROUND = 0x0000;
    static constexpr uint16_t COLOR_PANEL      = 0x2124; // dark grey tile
    static constexpr uint16_t COLOR_LABEL      = 0x8410; // mid grey
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
    //   toRgb565             FastLED colour to the 16 bit colour of the display.
    //   textColorOn          Black or white, whichever reads better on a background.
    //   positionFromColumn   Column of the colour bar to a colour position 0..255.
    //   columnFromPosition   The inverse, for the marker.
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

    static uint8_t positionFromColumn(int16_t column)
    {
        return column * 255 / (ROW_WIDTH - 1);
    }

    static int16_t columnFromPosition(uint8_t position)
    {
        return position * (ROW_WIDTH - 1) / 255;
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

        // Caption of the colour bar, the bar itself follows with update()
        _gfx->setTextSize(1);
        _gfx->setTextColor(COLOR_LABEL);
        _gfx->setCursor(ROW_X + TEXT_X, ROWS_Y + static_cast<int16_t>(Row::COLOR) * (ROW_HEIGHT + ROW_GAP) + LABEL_Y);
        _gfx->print("FARBE");

        update();
    }

    // ===========================================================================
    // ================================   TOUCH   ================================
    // ===========================================================================
    //
    //   > When the finger comes down
    //       - On a tile: select its next option.
    //       - On the colour bar: start a drag.
    //   > On every report of a drag
    //       - Set the colour from the x position.
    //
    //   Tiles only react to the first report of a touch, a resting finger would
    //   otherwise keep cycling.
    //

    void ControlScreen::handleTouch(const Input::TouchPoint &point)
    {
        // When the finger comes down
        if (point.isNewTouch)
        {
            // The gap below a row and the margins count to the nearest row
            const int16_t row = constrain((static_cast<int16_t>(point.y) - ROWS_Y) / (ROW_HEIGHT + ROW_GAP), 0,
                                          ROW_COUNT - 1);
            _isDraggingColor  = Row::COLOR == static_cast<Row>(row);

            // On a tile: select its next option.
            switch (static_cast<Row>(row))
            {
            case Row::POWER:  _strip->togglePower(); break;
            case Row::EFFECT: _strip->cycleMode();   break;
            case Row::TIMER:  _timer->cycleStep();   break;
            // On the colour bar: start a drag.
            case Row::COLOR:  break;
            }
        }

        // On every report of a drag
        if (!_isDraggingColor)
        {
            return;
        }
        // A finger left or right of the bar still reaches both ends
        const int16_t column = constrain(static_cast<int16_t>(point.x) - ROW_X, 0, ROW_WIDTH - 1);
        _strip->setColorPosition(positionFromColumn(column));
    }

    // ===========================================================================
    // ===============================   DRAWING   ===============================
    // ===========================================================================
    //
    //   update           Compares strip and timer with what is on screen and redraws
    //                    the rows that depend on a changed value.
    //   drawPowerTile    "AN" in the selected colour or "AUS" in grey.
    //   drawColorBar     The colour bar with the marker at the selected colour.
    //   drawEffectTile   Name of the effect.
    //   drawTimerTile    Preset and the time until the next switch.
    //   drawTile         One tile: caption, value and an optional detail line.
    //
    //   Which row depends on which value:
    //     Power tile    power, colour
    //     Colour bar    colour
    //     Effect tile   effect
    //     Timer tile    timer revision
    //

    void ControlScreen::update()
    {
        const bool isStripChanged = _isFirstDraw || _strip->revision() != _drawnRevision;
        const bool isTimerChanged = _isFirstDraw || _timer->revision() != _drawnTimerRevision;
        if (!isStripChanged && !isTimerChanged)
        {
            return;
        }
        // Throttled, a drag delivers reports faster than the display needs frames
        const uint32_t now = millis();
        if (!_isFirstDraw && now - _lastDrawMs < DRAW_INTERVAL_MS)
        {
            return;
        }
        _lastDrawMs         = now;
        _drawnRevision      = _strip->revision();
        _drawnTimerRevision = _timer->revision();

        const bool isPowerChanged = _isFirstDraw || _strip->isOn() != _drawnIsOn;
        const bool isColorChanged = _isFirstDraw || _strip->colorPosition() != _drawnColorPosition;
        const bool isModeChanged  = _isFirstDraw || _strip->mode() != _drawnMode;

        _drawnIsOn          = _strip->isOn();
        _drawnColorPosition = _strip->colorPosition();
        _drawnMode          = _strip->mode();
        _isFirstDraw        = false;

        if (isPowerChanged || isColorChanged)
        {
            drawPowerTile();
        }
        if (isColorChanged)
        {
            drawColorBar();
        }
        if (isModeChanged)
        {
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
            drawTile(Row::POWER, COLOR_PANEL, COLOR_WHITE, "LICHT", "AUS", nullptr);
            return;
        }
        const CRGB color = _strip->color();
        drawTile(Row::POWER, toRgb565(color), textColorOn(color), "LICHT", "AN", nullptr);
    }

    void ControlScreen::drawColorBar()
    {
        const int16_t barY         = ROWS_Y + static_cast<int16_t>(Row::COLOR) * (ROW_HEIGHT + ROW_GAP) + BAR_Y;
        const int16_t markerColumn = columnFromPosition(_drawnColorPosition);

        // Bar and marker in one pass column by column, so nothing flickers while dragging
        _gfx->startWrite();
        for (int16_t column = 0; column < ROW_WIDTH; column++)
        {
            const int16_t markerDistance = abs(column - markerColumn);
            uint16_t color;
            if (markerDistance <= MARKER_HALF)
            {
                color = COLOR_WHITE;
            }
            else if (markerDistance <= MARKER_OUTLINE)
            {
                color = COLOR_BLACK;
            }
            else
            {
                // The same mapping the strip uses, so the bar shows exactly what a tap selects
                color = toRgb565(Leds::StripController::colorAt(positionFromColumn(column)));
            }
            _gfx->writeFastVLine(ROW_X + column, barY, BAR_HEIGHT, color);
        }
        _gfx->endWrite();
    }

    void ControlScreen::drawEffectTile()
    {
        drawTile(Row::EFFECT, COLOR_PANEL, COLOR_WHITE, "EFFEKT", MODE_LABELS[static_cast<uint8_t>(_drawnMode)],
                 nullptr);
    }

    void ControlScreen::drawTimerTile()
    {
        if (0 == _timer->hours())
        {
            drawTile(Row::TIMER, COLOR_PANEL, COLOR_WHITE, "TIMER", "AUS", nullptr);
            return;
        }

        char value[8];
        char status[16];
        snprintf(value, sizeof(value), "%u H", _timer->hours());
        _timer->statusLine(status, sizeof(status));
        drawTile(Row::TIMER, COLOR_PANEL, COLOR_WHITE, "TIMER", value, status);
    }

    void ControlScreen::drawTile(Row row, uint16_t background, uint16_t foreground, const char *label,
                                 const char *value, const char *detail)
    {
        const int16_t y = ROWS_Y + static_cast<int16_t>(row) * (ROW_HEIGHT + ROW_GAP);

        _gfx->fillRoundRect(ROW_X, y, ROW_WIDTH, ROW_HEIGHT, TILE_RADIUS, background);
        _gfx->setTextColor(foreground);

        _gfx->setTextSize(1);
        _gfx->setCursor(ROW_X + TEXT_X, y + LABEL_Y);
        _gfx->print(label);

        _gfx->setTextSize(VALUE_SCALE);
        _gfx->setCursor(ROW_X + TEXT_X, y + VALUE_Y);
        _gfx->print(value);

        if (nullptr != detail)
        {
            _gfx->setTextSize(1);
            _gfx->setCursor(ROW_X + TEXT_X, y + DETAIL_Y);
            _gfx->print(detail);
        }
    }
}
