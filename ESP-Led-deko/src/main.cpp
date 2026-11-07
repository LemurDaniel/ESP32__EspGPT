// NOTE:
//   Entry point: candle flame on the WS2812B LEDs, daily timer, status display and BOOT
//   button on the Waveshare ESP32-C6-LCD-1.47. No WiFi, no web server.
//
//   pio run -e waveshare-c6-lcd147 --target upload
//
//   BOOT button: short press = flame on / off, long press = timer off -> 4 h -> 8 h -> 12 h.

//
//   Chapters:
//     START THE DEVICE   setup, LEDs first so the flame lights up right away.
//     RUN THE LOOP       loop, button, timer, LED frame and display.
//

#include <Arduino.h>

#include <display/StatusDisplay.h>
#include <input/BootButton.h>
#include <leds/LedController.h>
#include <timer/CandleTimer.h>

Leds::LedController LedStrip;
Display::StatusDisplay Screen;
Input::BootButton Button(BOOT_BUTTON_PIN);
Timers::CandleTimer Timer;

// ===========================================================================
// ==========================   START THE DEVICE   ===========================
// ===========================================================================
//
//   1. Hardware   Serial, LEDs, display, button and timer.
//

void setup()
{
    // ---------------------------------------------------------------------------
    // Hardware
    //   Serial, LEDs, display, button and timer.
    // ---------------------------------------------------------------------------

    Serial.begin(115200);
    LedStrip.begin();
    Screen.begin();
    Button.begin();
    Timer.begin();
}

// ===========================================================================
// ============================   RUN THE LOOP   =============================
// ===========================================================================
//
//   1. Button    Short press = flame on / off, long press = next timer step.
//   2. Timer     Switches the flame at the start and end of the daily on phase.
//   3. Render    One LED frame, paced inside LedController.
//   4. Display   Page on change, timer line, live preview of the frame.
//
//   A manual toggle between two timer switches stays until the next switch,
//   like the timer of a store-bought LED candle.
//

void loop()
{
    // ---------------------------------------------------------------------------
    // Button
    //   Short press = flame on / off, long press = next timer step.
    // ---------------------------------------------------------------------------

    switch (Button.poll())
    {
    case Input::ButtonEvent::SHORT_PRESS: LedStrip.togglePower(); break;
    case Input::ButtonEvent::LONG_PRESS:  Timer.cycleStep();      break;
    default:                              break;
    }

    // ---------------------------------------------------------------------------
    // Timer
    //   Switches the flame at the start and end of the daily on phase.
    // ---------------------------------------------------------------------------

    switch (Timer.poll())
    {
    case Timers::TimerEvent::TURN_ON:  LedStrip.setPower(true);  break;
    case Timers::TimerEvent::TURN_OFF: LedStrip.setPower(false); break;
    default:                           break;
    }

    // ---------------------------------------------------------------------------
    // Render
    //   One LED frame, paced inside LedController.
    // ---------------------------------------------------------------------------

    LedStrip.update();

    // ---------------------------------------------------------------------------
    // Display
    //   Page on change, timer line, live preview of the frame.
    // ---------------------------------------------------------------------------

    Screen.update(LedStrip.isOn(), LedStrip.revision(), LedStrip.pixels(), Timer.statusLine());

    delay(1);
}
