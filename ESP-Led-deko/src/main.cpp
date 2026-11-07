// NOTE:
//   Entry point: candle flame on the WS2812B LEDs, status display and BOOT button on the
//   Waveshare ESP32-C6-LCD-1.47. No WiFi, no web server.
//
//   pio run -e waveshare-c6-lcd147 --target upload
//
//   BOOT button: short press = flame on / off.

//
//   Chapters:
//     START THE DEVICE   setup, LEDs first so the flame lights up right away.
//     RUN THE LOOP       loop, button, LED frame and display.
//

#include <Arduino.h>

#include <display/StatusDisplay.h>
#include <input/BootButton.h>
#include <leds/LedController.h>

Leds::LedController LedStrip;
Display::StatusDisplay Screen;
Input::BootButton Button(BOOT_BUTTON_PIN);

// ===========================================================================
// ==========================   START THE DEVICE   ===========================
// ===========================================================================
//
//   1. Hardware   Serial, LEDs, display and button.
//

void setup()
{
    // ---------------------------------------------------------------------------
    // Hardware
    //   Serial, LEDs, display and button.
    // ---------------------------------------------------------------------------

    Serial.begin(115200);
    LedStrip.begin();
    Screen.begin();
    Button.begin();
}

// ===========================================================================
// ============================   RUN THE LOOP   =============================
// ===========================================================================
//
//   1. Button    Short press = flame on / off.
//   2. Render    One LED frame, paced inside LedController.
//   3. Display   Page on change, live preview of the frame.
//

void loop()
{
    // ---------------------------------------------------------------------------
    // Button
    //   Short press = flame on / off.
    // ---------------------------------------------------------------------------

    // A long press toggles as well, so holding the button a bit too long still works
    if (Input::ButtonEvent::NONE != Button.poll())
    {
        LedStrip.togglePower();
    }

    // ---------------------------------------------------------------------------
    // Render
    //   One LED frame, paced inside LedController.
    // ---------------------------------------------------------------------------

    LedStrip.update();

    // ---------------------------------------------------------------------------
    // Display
    //   Page on change, live preview of the frame.
    // ---------------------------------------------------------------------------

    Screen.update(LedStrip.isOn(), LedStrip.revision(), LedStrip.pixels());

    delay(1);
}
