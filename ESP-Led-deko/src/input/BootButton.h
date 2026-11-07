// NOTE:
//   Debounced push button with short and long press, used for the BOOT button (GPIO9).
//   Requires: a button that pulls the pin to GND when pressed (BOOT_BUTTON_PIN as build flag).
//
//   Input::BootButton button(BOOT_BUTTON_PIN);
//   button.begin();
//   switch (button.poll()) { case Input::ButtonEvent::SHORT_PRESS: ...; }   // in loop()

//
//   Chapters:
//     READ THE BUTTON   ButtonEvent and the BootButton class.
//

#pragma once

#include <Arduino.h>

namespace Input
{
    // ===========================================================================
    // ===========================   READ THE BUTTON   ===========================
    // ===========================================================================
    //
    //   Events      What poll() reports.
    //   Lifecycle   Constructor and begin.
    //   Polling     poll, debounce and press timing.
    //

    // ---------------------------------------------------------------------------
    // Events
    //   What poll() reports.
    // ---------------------------------------------------------------------------

    /** @brief Result of one poll(), at most one event per call. */
    enum class ButtonEvent : uint8_t
    {
        NONE,
        SHORT_PRESS, // released before LONG_PRESS_MS
        LONG_PRESS   // fired once while still held, the release afterwards is silent
    };

    class BootButton
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   Constructor and begin.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Binds the button to a pin, nothing is configured yet.
         * @param pin GPIO the button pulls to GND (e.g. `9` for BOOT).
         */
        explicit BootButton(uint8_t pin);

        /** @brief Sets the pin to input with pull-up. */
        void begin();

    private:
        const uint8_t _pin;

    // ---------------------------------------------------------------------------
    // Polling
    //   poll, debounce and press timing.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Reads the pin, call it on every loop() pass.
         * @return SHORT_PRESS on release, LONG_PRESS once the hold time is reached, else NONE.
         */
        ButtonEvent poll();

    private:
        static constexpr uint32_t DEBOUNCE_MS   = 30;
        static constexpr uint32_t LONG_PRESS_MS = 800;

        bool _isPressed       = false; // debounced state
        bool _lastRaw         = false;
        bool _hasFiredLong    = false;
        uint32_t _rawChangeMs = 0;
        uint32_t _pressedMs   = 0;
    };
}
