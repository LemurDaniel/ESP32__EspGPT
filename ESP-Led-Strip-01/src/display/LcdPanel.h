// NOTE:
//   The 1.47" JD9853 display (172x320, SPI) and its backlight.
//   Requires: GFX Library for Arduino, LCD_SCLK / LCD_MOSI / LCD_CS / LCD_DC / LCD_RST /
//   LCD_BL / SD_CS as build flags.
//
//   Display::LcdPanel panel;
//   panel.begin();                       // in setup()
//   panel.gfx().fillScreen(0x0000);      // draw with the Arduino_GFX API

//
//   Chapters:
//     DRIVE THE DISPLAY   The LcdPanel class.
//

#pragma once

#include <Arduino.h>

#include <Arduino_GFX_Library.h>

namespace Display
{
    // ===========================================================================
    // ==========================   DRIVE THE DISPLAY   ==========================
    // ===========================================================================
    //
    //   Lifecycle   begin, SPI bus, JD9853 register init and the backlight.
    //   Drawing     gfx, width and height.
    //

    class LcdPanel
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, SPI bus, JD9853 register init and the backlight.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Starts the display, clears it to black and turns the backlight on. */
        void begin();

        /**
         * @brief Sets the backlight brightness.
         * @param percent 0 (off) to 100 (full).
         */
        void setBacklight(uint8_t percent);

    // ---------------------------------------------------------------------------
    // Drawing
    //   gfx, width and height.
    // ---------------------------------------------------------------------------
    public:
        static constexpr uint16_t WIDTH  = 172;
        static constexpr uint16_t HEIGHT = 320;

        /** @brief The graphics context, only valid after begin(). */
        Arduino_GFX &gfx() { return *_gfx; }

    private:
        Arduino_DataBus *_bus = nullptr;
        Arduino_GFX *_gfx     = nullptr;
    };
}
