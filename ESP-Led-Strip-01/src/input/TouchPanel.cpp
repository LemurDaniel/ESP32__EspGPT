// NOTE:
//   Implementation of TouchPanel.h.
//   Protocol per the Waveshare demo (Arduino/libraries/esp_lcd_touch_axs5106l).

//
//   Chapters:
//     LIFECYCLE   begin.
//     POLLING     onInterrupt, poll and readRegister.
//

#include <input/TouchPanel.h>

namespace Input
{
    static constexpr uint8_t AXS5106L_ADDRESS    = 0x63;
    static constexpr uint8_t REGISTER_ID         = 0x08;
    static constexpr uint8_t REGISTER_TOUCH_DATA = 0x01;
    static constexpr size_t TOUCH_DATA_LENGTH    = 14;

    static constexpr uint32_t RESET_LOW_MS  = 200;
    static constexpr uint32_t RESET_WAIT_MS = 300;

    volatile bool TouchPanel::_hasReport = false;

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================

    void TouchPanel::begin(uint16_t width, uint16_t height)
    {
        _width  = width;
        _height = height;

        Wire.begin(TOUCH_SDA, TOUCH_SCL);

        // Hardware reset
        pinMode(TOUCH_RST, OUTPUT);
        digitalWrite(TOUCH_RST, LOW);
        delay(RESET_LOW_MS);
        digitalWrite(TOUCH_RST, HIGH);
        delay(RESET_WAIT_MS);

        // The controller pulls INT low for every report
        attachInterrupt(TOUCH_INT, onInterrupt, FALLING);

        uint8_t id[3] = {0};
        if (readRegister(REGISTER_ID, id, sizeof(id)))
        {
            Serial.printf("[TouchPanel] AXS5106L ready, id %02X %02X %02X\n", id[0], id[1], id[2]);
        }
    }

    // ===========================================================================
    // ===============================   POLLING   ===============================
    // ===========================================================================
    //
    //   A report is 14 bytes from register 0x01:
    //     [1]      number of touch points, 0 = finger lifted
    //     [2..3]   x of the first point, 12 bit: low nibble of [2] and [3]
    //     [4..5]   y of the first point, 12 bit: low nibble of [4] and [5]
    //   The x axis of the controller runs opposite to the display in portrait.
    //

    void IRAM_ATTR TouchPanel::onInterrupt()
    {
        _hasReport = true;
    }

    bool TouchPanel::poll(TouchPoint &point)
    {
        if (!_hasReport)
        {
            return false;
        }
        _hasReport = false;

        uint8_t data[TOUCH_DATA_LENGTH] = {0};
        if (!readRegister(REGISTER_TOUCH_DATA, data, sizeof(data)))
        {
            return false;
        }

        // Finger lifted
        if (0 == data[1])
        {
            _isTouching = false;
            return false;
        }

        // [0x_3, 0x21] => 0x321
        const uint16_t rawX = (static_cast<uint16_t>(data[2] & 0x0F) << 8) | data[3];
        const uint16_t rawY = (static_cast<uint16_t>(data[4] & 0x0F) << 8) | data[5];
        if (rawX >= _width || rawY >= _height)
        {
            Serial.printf("[TouchPanel] Report outside the panel ignored: %u / %u\n", rawX, rawY);
            return false;
        }

        const uint32_t now = millis();
        point.x            = _width - 1 - rawX;
        point.y            = rawY;
        point.isNewTouch   = !_isTouching || now - _lastReportMs > NEW_TOUCH_GAP_MS;

        _isTouching   = true;
        _lastReportMs = now;
        return true;
    }

    bool TouchPanel::readRegister(uint8_t reg, uint8_t *data, size_t length)
    {
        Wire.beginTransmission(AXS5106L_ADDRESS);
        Wire.write(reg);
        if (0 != Wire.endTransmission())
        {
            Serial.printf("[TouchPanel] I2C write of register 0x%02X failed\n", reg);
            return false;
        }

        Wire.requestFrom(AXS5106L_ADDRESS, length);
        if (static_cast<size_t>(Wire.available()) != length)
        {
            Serial.printf("[TouchPanel] I2C read of register 0x%02X failed\n", reg);
            return false;
        }
        Wire.readBytes(data, length);
        return true;
    }
}
