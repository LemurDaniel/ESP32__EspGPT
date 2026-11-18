// NOTE:
//   Implementation of LcdPanel.h.

//
//   Chapters:
//     LIFECYCLE   The JD9853 register init, begin and setBacklight.
//

#include <display/LcdPanel.h>

namespace Display
{
    // The glass is 240 columns wide in the controller, the 172 visible ones start at 34
    static constexpr uint8_t COLUMN_OFFSET = 34;

    static constexpr uint32_t BACKLIGHT_PWM_HZ   = 5000;
    static constexpr uint8_t BACKLIGHT_PWM_BITS  = 10;
    static constexpr uint8_t BACKLIGHT_DEFAULT   = 80; // percent

    // Vendor register init of the JD9853, taken 1:1 from the Waveshare demo
    // (ESP32-C6-Touch-LCD-1.47-Demo, Arduino/examples/06_lvgl_brightness, lcd_reg_init).
    // Arduino_GFX has no JD9853 class: the ST7789 class provides the drawing commands,
    // this sequence replaces its panel setup.
    static const uint8_t JD9853_INIT[] = {
        BEGIN_WRITE,
        WRITE_COMMAND_8, 0x11, // sleep out
        END_WRITE,
        DELAY, 120,

        BEGIN_WRITE,
        WRITE_C8_D16, 0xDF, 0x98, 0x53,
        WRITE_C8_D8, 0xB2, 0x23,

        WRITE_COMMAND_8, 0xB7,
        WRITE_BYTES, 4,
        0x00, 0x47, 0x00, 0x6F,

        WRITE_COMMAND_8, 0xBB,
        WRITE_BYTES, 6,
        0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,

        WRITE_C8_D16, 0xC0, 0x44, 0xA4,
        WRITE_C8_D8, 0xC1, 0x16,

        WRITE_COMMAND_8, 0xC3,
        WRITE_BYTES, 8,
        0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,

        WRITE_COMMAND_8, 0xC4,
        WRITE_BYTES, 12,
        0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,

        WRITE_COMMAND_8, 0xC8,
        WRITE_BYTES, 32,
        0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
        0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,

        WRITE_COMMAND_8, 0xD0,
        WRITE_BYTES, 5,
        0x04, 0x06, 0x6B, 0x0F, 0x00,

        WRITE_C8_D16, 0xD7, 0x00, 0x30,
        WRITE_C8_D8, 0xE6, 0x14,
        WRITE_C8_D8, 0xDE, 0x01,

        WRITE_COMMAND_8, 0xB7,
        WRITE_BYTES, 5,
        0x03, 0x13, 0xEF, 0x35, 0x35,

        WRITE_COMMAND_8, 0xC1,
        WRITE_BYTES, 3,
        0x14, 0x15, 0xC0,

        WRITE_C8_D16, 0xC2, 0x06, 0x3A,
        WRITE_C8_D16, 0xC4, 0x72, 0x12,
        WRITE_C8_D8, 0xBE, 0x00,
        WRITE_C8_D8, 0xDE, 0x02,

        WRITE_COMMAND_8, 0xE5,
        WRITE_BYTES, 3,
        0x00, 0x02, 0x00,

        WRITE_COMMAND_8, 0xE5,
        WRITE_BYTES, 3,
        0x01, 0x02, 0x00,

        WRITE_C8_D8, 0xDE, 0x00,
        WRITE_C8_D8, 0x35, 0x00,
        WRITE_C8_D8, 0x3A, 0x05, // 16 bit per pixel

        WRITE_COMMAND_8, 0x2A,
        WRITE_BYTES, 4,
        0x00, 0x22, 0x00, 0xCD,

        WRITE_COMMAND_8, 0x2B,
        WRITE_BYTES, 4,
        0x00, 0x00, 0x01, 0x3F,

        WRITE_C8_D8, 0xDE, 0x02,

        WRITE_COMMAND_8, 0xE5,
        WRITE_BYTES, 3,
        0x00, 0x02, 0x00,

        WRITE_C8_D8, 0xDE, 0x00,
        WRITE_C8_D8, 0x36, 0x00,
        WRITE_COMMAND_8, 0x21, // inversion on
        END_WRITE,

        DELAY, 10,

        BEGIN_WRITE,
        WRITE_COMMAND_8, 0x29, // display on
        END_WRITE,
    };

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   begin          1. TF card off the bus, 2. SPI bus and panel, 3. register
    //                  init, 4. backlight.
    //   setBacklight   PWM duty from a percentage.
    //

    void LcdPanel::begin()
    {
        // The TF card shares SCLK and MOSI with the display, its chip select stays deasserted
        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);

        // SPI bus and panel
        _bus = new Arduino_HWSPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI);
        _gfx = new Arduino_ST7789(_bus, LCD_RST, 0 /* rotation */, false /* IPS */, WIDTH, HEIGHT,
                                  COLUMN_OFFSET, 0, COLUMN_OFFSET, 0);
        if (!_gfx->begin())
        {
            Serial.println("[LcdPanel] gfx begin failed, the display stays dark");
        }

        // Register init, then the same order as the demo: rotation and a black screen
        _bus->batchOperation(JD9853_INIT, sizeof(JD9853_INIT));
        _gfx->setRotation(0);
        _gfx->fillScreen(0x0000);

        // Backlight last, so the panel never shows the uninitialised frame
        ledcAttach(LCD_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS);
        setBacklight(BACKLIGHT_DEFAULT);

        Serial.printf("[LcdPanel] JD9853 %ux%u ready\n", WIDTH, HEIGHT);
    }

    void LcdPanel::setBacklight(uint8_t percent)
    {
        const uint32_t maxDuty = (1u << BACKLIGHT_PWM_BITS) - 1;
        ledcWrite(LCD_BL, maxDuty * min<uint8_t>(percent, 100) / 100);
    }
}
