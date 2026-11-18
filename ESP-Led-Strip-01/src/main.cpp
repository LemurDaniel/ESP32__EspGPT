// NOTE:
//   Entry point: WS2812 strip with a touch UI on the Waveshare ESP32-C6-Touch-LCD-1.47.
//   Requires: strip on VBUS / GND, DIN via 330 Ohm on LED_DATA_PIN (see platformio.ini).
//
//   pio run -e waveshare-c6-touch-lcd147 --target upload
//   pio device monitor --baud 115200
//
//   Touch: header = on / off, sliders = hue, saturation, brightness, buttons = effect and timer.

//
//   Chapters:
//     START THE DEVICE   setup, strip first so it lights up before the display is ready.
//     RUN THE LOOP       loop, touch, timer, LED frame and display.
//

#include <Arduino.h>

#include <display/LcdPanel.h>
#include <input/TouchPanel.h>
#include <leds/StripController.h>
#include <timer/DailyTimer.h>
#include <ui/ControlScreen.h>

Leds::StripController Strip;
Timers::DailyTimer Timer;
Display::LcdPanel Panel;
Input::TouchPanel Touch;
Ui::ControlScreen Screen;

// ===========================================================================
// ==========================   START THE DEVICE   ===========================
// ===========================================================================

void setup()
{
    Serial.begin(115200);

    Strip.begin();
    Panel.begin();
    Touch.begin(Display::LcdPanel::WIDTH, Display::LcdPanel::HEIGHT);
    Timer.begin();
    Screen.begin(Panel.gfx(), Strip, Timer);
}

// ===========================================================================
// ============================   RUN THE LOOP   =============================
// ===========================================================================
//
//   1. Touch     A touch report changes the strip settings or the timer preset.
//   2. Timer     Switches the strip at the edges of the daily on time.
//   3. Render    One LED frame, paced inside StripController.
//   4. Display   Redraws the controls whose values changed.
//
//   Everything runs here in the loop task, so no class needs locking.
//

void loop()
{
    // Touch
    Input::TouchPoint point;
    if (Touch.poll(point))
    {
        Screen.handleTouch(point);
    }

    // Timer
    switch (Timer.poll())
    {
    case Timers::TimerEvent::TURN_ON:  Strip.setPower(true);  break;
    case Timers::TimerEvent::TURN_OFF: Strip.setPower(false); break;
    default:                           break;
    }

    // Render
    Strip.update();

    // Display
    Screen.update();

    delay(1);
}
