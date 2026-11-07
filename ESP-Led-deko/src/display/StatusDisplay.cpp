// NOTE:
//   Implementation of StatusDisplay.h.

//
//   Chapters:
//     LIFECYCLE   Constructor, begin and update.
//     PAGE        The static layout.
//     PREVIEW     The live LED dots, arranged as a ring like the Freenove module.
//

#include <display/StatusDisplay.h>

namespace Display
{
    // Theme, RGB565
    static constexpr uint16_t COLOR_BACKGROUND = 0x0000;
    static constexpr uint16_t COLOR_TEXT       = 0xFFFF;
    static constexpr uint16_t COLOR_MUTED      = 0x8410;
    static constexpr uint16_t COLOR_RING       = 0x2104;
    static constexpr uint16_t COLOR_FLAME      = 0xFC60; // warm orange

    // Layout, portrait 172x320
    static constexpr int16_t MARGIN        = 10;
    static constexpr int16_t STATUS_TOP    = 45;
    static constexpr int16_t RING_CENTER_X = 86;  // 172 / 2
    static constexpr int16_t RING_CENTER_Y = 180;
    static constexpr int16_t RING_RADIUS   = 58;
    static constexpr int16_t DOT_RADIUS    = 14;

    // Dot index => position on the ring, index 0 at the top, clockwise.
    // 8 LEDs => one dot every 45 degrees.
    static int16_t dotCenterX(uint16_t index)
    {
        const float angle = TWO_PI * index / LED_COUNT;
        return RING_CENTER_X + static_cast<int16_t>(RING_RADIUS * sinf(angle));
    }
    static int16_t dotCenterY(uint16_t index)
    {
        const float angle = TWO_PI * index / LED_COUNT;
        return RING_CENTER_Y - static_cast<int16_t>(RING_RADIUS * cosf(angle));
    }

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   StatusDisplay   Binds the panel to the shared SPI bus.
    //   begin           Starts SPI on the LCD pins, the panel and the backlight.
    //   update          Page on change, preview at PREVIEW_INTERVAL_MS.
    //

    StatusDisplay::StatusDisplay() : _tft(&SPI, LCD_CS, LCD_DC, LCD_RST)
    {
    }

    void StatusDisplay::begin()
    {
        SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
        // 172x320 sets the 34 px column offset of this panel inside the library
        _tft.init(WIDTH, HEIGHT);
        _tft.setRotation(0);
        _tft.fillScreen(COLOR_BACKGROUND);

        pinMode(LCD_BL, OUTPUT);
        analogWrite(LCD_BL, BACKLIGHT_DUTY);
        Serial.printf("[StatusDisplay] Panel ready, preview ring of %d LEDs\n", LED_COUNT);
    }

    void StatusDisplay::update(bool isOn, uint32_t revision, const CRGB *pixels)
    {
        // Page
        if (!_hasDrawn || revision != _drawnRevision)
        {
            drawPage(isOn);
            _hasDrawn      = true;
            _drawnRevision = revision;
        }

        // Preview
        const uint32_t now = millis();
        if (!_isPreviewStale && now - _lastPreviewMs < PREVIEW_INTERVAL_MS)
        {
            return;
        }
        _lastPreviewMs = now;
        drawPreviewFrame(pixels);
    }

    // ===========================================================================
    // ================================   PAGE   =================================
    // ===========================================================================
    //
    //   drawPage   Title, state, empty dot rings and the button hint.
    //

    void StatusDisplay::drawPage(bool isOn)
    {
        _tft.fillScreen(COLOR_BACKGROUND);

        // Title
        _tft.setTextColor(COLOR_TEXT);
        _tft.setTextSize(2);
        _tft.setCursor(MARGIN, MARGIN);
        _tft.print("LED Deko");

        // State
        _tft.setTextSize(3);
        _tft.setCursor(MARGIN, STATUS_TOP);
        _tft.setTextColor(isOn ? COLOR_FLAME : COLOR_MUTED);
        _tft.print(isOn ? "Kerze" : "aus");

        // Dot rings, so dark LEDs stay visible on the black background
        for (uint16_t index = 0; index < LED_COUNT; index++)
        {
            _tft.drawCircle(dotCenterX(index), dotCenterY(index), DOT_RADIUS + 1, COLOR_RING);
        }
        _isPreviewStale = true;

        // Button hint
        _tft.setTextSize(1);
        _tft.setTextColor(COLOR_MUTED);
        _tft.setCursor(MARGIN, HEIGHT - 20);
        _tft.print("BOOT: an / aus");
    }

    // ===========================================================================
    // ===============================   PREVIEW   ===============================
    // ===========================================================================
    //
    //   drawPreviewFrame   Redraws the dots whose color changed, all after drawPage.
    //   drawDot            Fills one dot inside its ring.
    //

    void StatusDisplay::drawPreviewFrame(const CRGB *pixels)
    {
        for (uint16_t index = 0; index < LED_COUNT; index++)
        {
            const uint16_t color = _tft.color565(pixels[index].r, pixels[index].g, pixels[index].b);
            if (!_isPreviewStale && color == _dotColors[index])
            {
                continue;
            }
            drawDot(index, color);
            _dotColors[index] = color;
        }
        _isPreviewStale = false;
    }

    void StatusDisplay::drawDot(uint16_t index, uint16_t color)
    {
        _tft.fillCircle(dotCenterX(index), dotCenterY(index), DOT_RADIUS, color);
    }
}
