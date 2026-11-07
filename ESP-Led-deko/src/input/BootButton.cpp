// NOTE:
//   Implementation of BootButton.h.

//
//   Chapters:
//     LIFECYCLE   Constructor and begin.
//     POLLING     poll with debounce and press timing.
//

#include <input/BootButton.h>

namespace Input
{
    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   BootButton   Stores the pin.
    //   begin        Input with pull-up, pressed reads LOW.
    //

    BootButton::BootButton(uint8_t pin) : _pin(pin)
    {
    }

    void BootButton::begin()
    {
        pinMode(_pin, INPUT_PULLUP);
    }

    // ===========================================================================
    // ===============================   POLLING   ===============================
    // ===========================================================================
    //
    //   poll   Debounces the pin, then reports short and long presses.
    //
    //   > Raw level changed
    //       - Restart the debounce timer, report nothing.
    //   > Level stable for DEBOUNCE_MS and different from the debounced state
    //       - Pressed:  remember the press time.
    //       - Released: SHORT_PRESS, unless the long press already fired.
    //   > Still held after LONG_PRESS_MS
    //       - LONG_PRESS, once per press.
    //

    ButtonEvent BootButton::poll()
    {
        const uint32_t now  = millis();
        const bool isRawLow = LOW == digitalRead(_pin);

        // Raw level changed
        if (isRawLow != _lastRaw)
        {
            _lastRaw     = isRawLow;
            _rawChangeMs = now;
            return ButtonEvent::NONE;
        }

        // Level stable for DEBOUNCE_MS and different from the debounced state
        if (now - _rawChangeMs >= DEBOUNCE_MS && isRawLow != _isPressed)
        {
            _isPressed = isRawLow;
            // Pressed: remember the press time.
            if (_isPressed)
            {
                _pressedMs    = now;
                _hasFiredLong = false;
                return ButtonEvent::NONE;
            }
            // Released: SHORT_PRESS, unless the long press already fired.
            return _hasFiredLong ? ButtonEvent::NONE : ButtonEvent::SHORT_PRESS;
        }

        // Still held after LONG_PRESS_MS
        if (_isPressed && !_hasFiredLong && now - _pressedMs >= LONG_PRESS_MS)
        {
            _hasFiredLong = true;
            return ButtonEvent::LONG_PRESS;
        }

        return ButtonEvent::NONE;
    }
}
