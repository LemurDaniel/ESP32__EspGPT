// NOTE:
//   Implementation of ControlScreen.h.

//
//   Chapters:
//     HELPERS     Colour conversion and the slider value mapping.
//     LIFECYCLE   begin.
//     TOUCH       handleTouch, applySlider and pressButton.
//     DRAWING     update and the draw functions of the parts.
//

#include <ui/ControlScreen.h>

#include <display/LcdPanel.h>

namespace Ui
{
    // Layout, see the chapter description in ControlScreen.h
    static constexpr int16_t SCREEN_WIDTH = Display::LcdPanel::WIDTH;

    static constexpr int16_t TITLE_X     = 8;
    static constexpr int16_t TITLE_Y     = 12;
    static constexpr int16_t PILL_X      = 122;
    static constexpr int16_t PILL_Y      = 8;
    static constexpr int16_t PILL_WIDTH  = 42;
    static constexpr int16_t PILL_HEIGHT = 24;

    static constexpr int16_t SLIDERS_Y       = 40;
    static constexpr int16_t SLIDER_ROW      = 54; // height of one slider row incl. label
    static constexpr int16_t SLIDER_LABEL_Y  = 4;  // inside the row
    static constexpr int16_t SLIDER_BAR_Y    = 18; // inside the row
    static constexpr int16_t SLIDER_X        = 8;
    static constexpr int16_t SLIDER_WIDTH    = 156;
    static constexpr int16_t SLIDER_HEIGHT   = 28;
    static constexpr int16_t SLIDER_COUNT    = 3;
    static constexpr int16_t MARKER_HALF     = 1; // white core of the marker, columns left and right
    static constexpr int16_t MARKER_OUTLINE  = 3; // black outline around it

    static constexpr int16_t BUTTONS_Y     = 208;
    static constexpr int16_t BUTTON_X      = 6;
    static constexpr int16_t BUTTON_WIDTH  = 76;
    static constexpr int16_t BUTTON_HEIGHT = 32;
    static constexpr int16_t BUTTON_GAP    = 8;
    static constexpr int16_t BUTTON_RADIUS = 8;
    static constexpr int16_t BUTTON_ROWS   = 3;
    static constexpr int16_t LINE_GAP      = 3; // between the two text lines of a button

    // The grid holds the effects in StripMode order, the timer takes the last cell
    static constexpr uint8_t TIMER_BUTTON_INDEX = Leds::STRIP_MODE_COUNT;

    // Default 6x8 font
    static constexpr int16_t CHAR_WIDTH  = 6;
    static constexpr int16_t CHAR_HEIGHT = 8;

    // RGB565
    static constexpr uint16_t COLOR_BACKGROUND = 0x0000;
    static constexpr uint16_t COLOR_PANEL      = 0x2124; // dark grey, inactive buttons and pill
    static constexpr uint16_t COLOR_TEXT       = 0xFFFF;
    static constexpr uint16_t COLOR_LABEL      = 0x8410; // mid grey
    static constexpr uint16_t COLOR_ON_ACCENT  = 0x0000; // text on an accent coloured part

    // Below this the strip looks off, the slider would feel broken at its left end
    static constexpr uint8_t BRIGHTNESS_MIN = 8;

    // The display font has no umlauts. Indexed by Slider and by Leds::StripMode.
    static constexpr const char *SLIDER_LABELS[SLIDER_COUNT]          = {"FARBTON", "SAETTIGUNG", "HELLIGKEIT"};
    static constexpr const char *MODE_LABELS[Leds::STRIP_MODE_COUNT] = {"FARBE", "REGENBOGEN", "ATMEN", "KOMET", "KERZE"};

    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   toRgb565           FastLED colour to the 16 bit colour of the display.
    //   valueFromColumn    Slider column to a value between min and 255.
    //   columnFromValue    The inverse, for the marker.
    //

    static uint16_t toRgb565(const CRGB &color)
    {
        return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
    }

    static uint8_t valueFromColumn(int16_t column, uint8_t min)
    {
        return min + column * (255 - min) / (SLIDER_WIDTH - 1);
    }

    static int16_t columnFromValue(uint8_t value, uint8_t min)
    {
        if (value <= min)
        {
            return 0;
        }
        return (value - min) * (SLIDER_WIDTH - 1) / (255 - min);
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

        // Title
        _gfx->setTextSize(2);
        _gfx->setTextColor(COLOR_TEXT);
        _gfx->setCursor(TITLE_X, TITLE_Y);
        _gfx->print("LED STRIP");

        // Slider labels
        _gfx->setTextSize(1);
        _gfx->setTextColor(COLOR_LABEL);
        for (int16_t index = 0; index < SLIDER_COUNT; index++)
        {
            _gfx->setCursor(SLIDER_X, SLIDERS_Y + index * SLIDER_ROW + SLIDER_LABEL_Y);
            _gfx->print(SLIDER_LABELS[index]);
        }

        update();
    }

    // ===========================================================================
    // ================================   TOUCH   ================================
    // ===========================================================================
    //
    //   > When the finger comes down
    //       - On the header: toggle the power.
    //       - On the buttons: select that effect, or the next timer preset.
    //       - On a slider row: remember the slider.
    //   > On every report while a slider is remembered
    //       - Set its value from the x position.
    //
    //   Header and buttons only react to the first report of a touch, a resting
    //   finger would otherwise toggle the power with every report.
    //

    void ControlScreen::handleTouch(const Input::TouchPoint &point)
    {
        // When the finger comes down
        if (point.isNewTouch)
        {
            _activeSlider = NO_SLIDER;

            // On the header: toggle the power.
            if (point.y < SLIDERS_Y)
            {
                _strip->togglePower();
                return;
            }
            // On the buttons: select that effect, or the next timer preset.
            if (point.y >= BUTTONS_Y)
            {
                pressButton(point);
                return;
            }
            // On a slider row: remember the slider. The gap above the buttons belongs to the last row.
            _activeSlider = min<int16_t>((point.y - SLIDERS_Y) / SLIDER_ROW, SLIDER_COUNT - 1);
        }

        // On every report while a slider is remembered
        if (NO_SLIDER == _activeSlider)
        {
            return;
        }
        applySlider(static_cast<Slider>(_activeSlider), point.x);
    }

    void ControlScreen::applySlider(Slider slider, uint16_t x)
    {
        // A finger left or right of the bar still reaches both ends
        const int16_t column = constrain(static_cast<int16_t>(x) - SLIDER_X, 0, SLIDER_WIDTH - 1);

        switch (slider)
        {
        case Slider::HUE:        _strip->setHue(valueFromColumn(column, 0));                     break;
        case Slider::SATURATION: _strip->setSaturation(valueFromColumn(column, 0));              break;
        case Slider::BRIGHTNESS: _strip->setBrightness(valueFromColumn(column, BRIGHTNESS_MIN)); break;
        }
    }

    void ControlScreen::pressButton(const Input::TouchPoint &point)
    {
        // The gaps between the buttons count to the nearest one
        const uint8_t column = point.x >= SCREEN_WIDTH / 2 ? 1 : 0;
        const uint8_t row    = min<int16_t>((point.y - BUTTONS_Y) / (BUTTON_HEIGHT + BUTTON_GAP), BUTTON_ROWS - 1);
        const uint8_t index  = row * 2 + column;

        if (TIMER_BUTTON_INDEX == index)
        {
            _timer->cycleStep();
            return;
        }
        _strip->setMode(static_cast<Leds::StripMode>(index));
    }

    // ===========================================================================
    // ===============================   DRAWING   ===============================
    // ===========================================================================
    //
    //   update            Compares strip and timer with what is on screen and redraws
    //                     the parts that depend on a changed value.
    //   drawPowerPill     "AN" in the accent colour or "AUS" in grey.
    //   drawSlider        Gradient bar with the marker at the current value.
    //   drawModeButtons   The five effects, the active one in the accent colour.
    //   drawTimerButton   Preset and the time until the next switch.
    //   drawButton        One cell of the button grid with one or two text lines.
    //   accentColor       The colour picked with the sliders, at full brightness.
    //
    //   Which part depends on which value:
    //     Power pill          power, hue, saturation
    //     Hue slider          hue
    //     Saturation slider   hue, saturation
    //     Brightness slider   hue, saturation, brightness
    //     Mode buttons        mode, hue, saturation
    //     Timer button        timer revision, hue, saturation
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

        const bool isPowerChanged      = _isFirstDraw || _strip->isOn() != _drawn.isOn;
        const bool isModeChanged       = _isFirstDraw || _strip->mode() != _drawn.mode;
        const bool isHueChanged        = _isFirstDraw || _strip->hue() != _drawn.hue;
        const bool isColorChanged      = isHueChanged || _strip->saturation() != _drawn.saturation;
        const bool isBrightnessChanged = _isFirstDraw || _strip->brightness() != _drawn.brightness;

        _drawn.isOn       = _strip->isOn();
        _drawn.mode       = _strip->mode();
        _drawn.hue        = _strip->hue();
        _drawn.saturation = _strip->saturation();
        _drawn.brightness = _strip->brightness();
        _isFirstDraw      = false;

        if (isPowerChanged || isColorChanged)
        {
            drawPowerPill();
        }
        if (isHueChanged)
        {
            drawSlider(Slider::HUE);
        }
        if (isColorChanged)
        {
            drawSlider(Slider::SATURATION);
        }
        if (isColorChanged || isBrightnessChanged)
        {
            drawSlider(Slider::BRIGHTNESS);
        }
        if (isModeChanged || isColorChanged)
        {
            drawModeButtons();
        }
        if (isTimerChanged || isColorChanged)
        {
            drawTimerButton();
        }
    }

    void ControlScreen::drawPowerPill()
    {
        const char *label = _drawn.isOn ? "AN" : "AUS";

        _gfx->fillRoundRect(PILL_X, PILL_Y, PILL_WIDTH, PILL_HEIGHT, PILL_HEIGHT / 2,
                            _drawn.isOn ? accentColor() : COLOR_PANEL);
        _gfx->setTextSize(1);
        _gfx->setTextColor(_drawn.isOn ? COLOR_ON_ACCENT : COLOR_TEXT);
        _gfx->setCursor(PILL_X + (PILL_WIDTH - static_cast<int16_t>(strlen(label)) * CHAR_WIDTH) / 2,
                        PILL_Y + (PILL_HEIGHT - CHAR_HEIGHT) / 2);
        _gfx->print(label);
    }

    void ControlScreen::drawSlider(Slider slider)
    {
        const int16_t barY = SLIDERS_Y + static_cast<int16_t>(slider) * SLIDER_ROW + SLIDER_BAR_Y;

        int16_t markerColumn = 0;
        switch (slider)
        {
        case Slider::HUE:        markerColumn = columnFromValue(_drawn.hue, 0);                     break;
        case Slider::SATURATION: markerColumn = columnFromValue(_drawn.saturation, 0);              break;
        case Slider::BRIGHTNESS: markerColumn = columnFromValue(_drawn.brightness, BRIGHTNESS_MIN); break;
        }

        // Bar and marker in one pass column by column, so nothing flickers while dragging
        _gfx->startWrite();
        for (int16_t column = 0; column < SLIDER_WIDTH; column++)
        {
            const int16_t markerDistance = abs(column - markerColumn);
            uint16_t color;
            if (markerDistance <= MARKER_HALF)
            {
                color = COLOR_TEXT;
            }
            else if (markerDistance <= MARKER_OUTLINE)
            {
                color = COLOR_BACKGROUND;
            }
            else
            {
                // The gradient shows what the slider does to the colour picked so far
                const uint8_t value = valueFromColumn(column, 0);
                switch (slider)
                {
                case Slider::HUE:        color = toRgb565(CHSV(value, 255, 255));                    break;
                case Slider::SATURATION: color = toRgb565(CHSV(_drawn.hue, value, 255));             break;
                case Slider::BRIGHTNESS: color = toRgb565(CHSV(_drawn.hue, _drawn.saturation, value)); break;
                }
            }
            _gfx->writeFastVLine(SLIDER_X + column, barY, SLIDER_HEIGHT, color);
        }
        _gfx->endWrite();
    }

    void ControlScreen::drawModeButtons()
    {
        for (uint8_t index = 0; index < Leds::STRIP_MODE_COUNT; index++)
        {
            drawButton(index, index == static_cast<uint8_t>(_drawn.mode), MODE_LABELS[index], nullptr);
        }
    }

    void ControlScreen::drawTimerButton()
    {
        const bool isActive = 0 != _timer->hours();

        char title[12];
        char status[16];
        if (isActive)
        {
            snprintf(title, sizeof(title), "TIMER %uH", _timer->hours());
        }
        else
        {
            snprintf(title, sizeof(title), "TIMER AUS");
        }
        _timer->statusLine(status, sizeof(status));

        drawButton(TIMER_BUTTON_INDEX, isActive, title, isActive ? status : nullptr);
    }

    void ControlScreen::drawButton(uint8_t index, bool isActive, const char *line1, const char *line2)
    {
        const int16_t x = BUTTON_X + (index % 2) * (BUTTON_WIDTH + BUTTON_GAP);
        const int16_t y = BUTTONS_Y + (index / 2) * (BUTTON_HEIGHT + BUTTON_GAP);

        _gfx->fillRoundRect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_RADIUS, isActive ? accentColor() : COLOR_PANEL);
        _gfx->setTextSize(1);
        _gfx->setTextColor(isActive ? COLOR_ON_ACCENT : COLOR_TEXT);

        // One line sits in the middle, two lines share it
        const int16_t textHeight = nullptr == line2 ? CHAR_HEIGHT : 2 * CHAR_HEIGHT + LINE_GAP;
        const int16_t textY      = y + (BUTTON_HEIGHT - textHeight) / 2;

        _gfx->setCursor(x + (BUTTON_WIDTH - static_cast<int16_t>(strlen(line1)) * CHAR_WIDTH) / 2, textY);
        _gfx->print(line1);
        if (nullptr != line2)
        {
            _gfx->setCursor(x + (BUTTON_WIDTH - static_cast<int16_t>(strlen(line2)) * CHAR_WIDTH) / 2,
                            textY + CHAR_HEIGHT + LINE_GAP);
            _gfx->print(line2);
        }
    }

    uint16_t ControlScreen::accentColor() const
    {
        return toRgb565(CHSV(_drawn.hue, _drawn.saturation, 255));
    }
}
