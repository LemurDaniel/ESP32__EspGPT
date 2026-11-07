// NOTE:
//   Daily candle timer: on for 4, 8 or 12 hours, then off until the same time the next day.
//   The cycle starts when the timer is set (or at boot with a stored timer), there is no
//   real-time clock. The selected step is persisted in NVS.
//   Requires: Preferences (NVS).
//
//   Timers::CandleTimer timer;
//   timer.begin();                                       // in setup()
//   timer.cycleStep();                                   // on long press
//   switch (timer.poll()) { case Timers::TimerEvent::TURN_ON: ...; }   // in loop()

//
//   Chapters:
//     SWITCH THE CANDLE DAILY   TimerEvent and the CandleTimer class.
//

#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace Timers
{
    // ===========================================================================
    // =======================   SWITCH THE CANDLE DAILY   =======================
    // ===========================================================================
    //
    //   Events        What poll() reports.
    //   Lifecycle     begin, loads the stored step and starts the cycle.
    //   Steps         cycleStep and hours, off -> 4 h -> 8 h -> 12 h -> off.
    //   Schedule      poll and the phase of the daily cycle.
    //   Status        statusLine for the display.
    //

    // ---------------------------------------------------------------------------
    // Events
    //   What poll() reports.
    // ---------------------------------------------------------------------------

    /** @brief Result of one poll(), reported once at each switch of the daily cycle. */
    enum class TimerEvent : uint8_t
    {
        NONE,
        TURN_ON, // the on phase began (also right after the timer was set)
        TURN_OFF // the on phase ended
    };

    class CandleTimer
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, loads the stored step and starts the cycle.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Loads the stored step, a set timer starts its cycle now. */
        void begin();

    // ---------------------------------------------------------------------------
    // Steps
    //   cycleStep and hours, off -> 4 h -> 8 h -> 12 h -> off.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Advances to the next step and restarts the cycle now.
         * @return The new on duration in hours, 0 when the timer is off.
         */
        uint8_t cycleStep();

        /** @brief The on duration in hours, 0 when the timer is off. */
        uint8_t hours() const;

    private:
        static constexpr uint8_t STEP_HOURS[] = {0, 4, 8, 12};
        static constexpr uint8_t STEP_COUNT   = sizeof(STEP_HOURS) / sizeof(STEP_HOURS[0]);

        void save();

        Preferences _prefs;
        uint8_t _stepIndex = 0;

    // ---------------------------------------------------------------------------
    // Schedule
    //   poll and the phase of the daily cycle.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Checks the daily cycle, call it on every loop() pass.
         * @return TURN_ON or TURN_OFF once at each switch, else NONE (always NONE while off).
         */
        TimerEvent poll();

    private:
        static constexpr uint64_t HOUR_MS = 60ULL * 60ULL * 1000ULL;
        static constexpr uint64_t DAY_MS  = 24ULL * HOUR_MS;

        /** @brief Milliseconds since boot, 64 bit so it never wraps (millis() wraps after 49 days). */
        static uint64_t nowMs();

        /** @brief Position inside the current day of the cycle, 0..DAY_MS-1. */
        uint64_t phaseMs() const;

        bool isInOnPhase() const;

        uint64_t _cycleStartMs = 0;
        bool _wasInOnPhase     = false; // false after a (re)start, so the first poll reports TURN_ON

    // ---------------------------------------------------------------------------
    // Status
    //   statusLine for the display.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief One short line for the display, changes at most once per minute.
         * @return e.g. `"Timer aus"`, `"Timer 8h  aus in 3:12"`, `"Timer 8h  an in 15:48"`.
         */
        String statusLine() const;
    };
}
