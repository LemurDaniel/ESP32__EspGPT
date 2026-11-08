// NOTE:
//   Daily candle timer on wall-clock time: burns for 1..23 hours from a start time
//   (e.g. 16:00 for 8 h => 16:00-24:00 every day). Duration and start time are persisted in NVS.
//   The button cycles the presets off -> 4 -> 8 -> 12 h, the web UI sets any full hour.
//   Requires: the local time from SNTP (configTzTime in setup) and Preferences (NVS).
//   Until the clock is synced the timer pauses and reports nothing.
//
//   Timers::CandleTimer timer;
//   timer.begin();                                       // in setup()
//   timer.cycleStep();                                   // on long press, starts now
//   timer.configure(8, 16 * 60);                         // 8 h from 16:00
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
    //   Lifecycle     begin, loads step and start time.
    //   Settings      cycleStep, configure and the getters.
    //   Schedule      poll and the phase of the day.
    //   Status        statusLine for display and web.
    //

    // ---------------------------------------------------------------------------
    // Events
    //   What poll() reports.
    // ---------------------------------------------------------------------------

    /** @brief Result of one poll(), reported at each switch and once after boot or a change. */
    enum class TimerEvent : uint8_t
    {
        NONE,
        TURN_ON, // inside the burn time
        TURN_OFF // outside the burn time
    };

    class CandleTimer
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, loads step and start time.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Loads the stored step and start time from NVS. */
        void begin();

    // ---------------------------------------------------------------------------
    // Settings
    //   cycleStep, configure and the getters.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Button presets in hours, 0 = timer off. */
        static constexpr uint8_t PRESET_HOURS[] = {0, 4, 8, 12};

        /** @brief Longest burn duration, 24 h would mean "always on". */
        static constexpr uint8_t MAX_HOURS = 23;

        /** @brief Minutes in one day, start times range 0..MINUTES_PER_DAY-1. */
        static constexpr uint16_t MINUTES_PER_DAY = 24 * 60;

        /**
         * @brief Advances to the next preset above the current duration (wraps to off),
         *        the start time becomes the current time (if synced).
         * @return The new burn duration in hours, 0 when the timer is off.
         */
        uint8_t cycleStep();

        /**
         * @brief Sets duration and start time together.
         * @param hours        0..MAX_HOURS, e.g. `8`. 0 turns the timer off.
         * @param startMinutes Minute of the day, e.g. `16 * 60` for 16:00.
         * @return True when both are valid, false leaves the timer unchanged.
         */
        bool configure(uint8_t hours, uint16_t startMinutes);

        /** @brief True for 0..MAX_HOURS. */
        static bool isValidHours(uint8_t hours);

        /** @brief The burn duration in hours, 0 when the timer is off. */
        uint8_t hours() const;

        /** @brief The start time as minute of the day. */
        uint16_t startMinutes() const;

    private:
        static constexpr uint16_t DEFAULT_START_MINUTES = 17 * 60; // 17:00

        /** @brief The one real implementation: stores duration and start, saves, re-evaluates. */
        void apply(uint8_t burnHours, uint16_t startMinutes);
        void save();

        Preferences _prefs;
        uint8_t _hours         = 0;
        uint16_t _startMinutes = DEFAULT_START_MINUTES;

    // ---------------------------------------------------------------------------
    // Schedule
    //   poll and the phase of the day.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Checks the burn time, call it on every loop() pass.
         * @return TURN_ON / TURN_OFF at each switch and once after boot or a change, else NONE.
         */
        TimerEvent poll();

        /** @brief True once SNTP delivered a plausible time. */
        static bool isClockSynced();

    private:
        static constexpr uint32_t SECONDS_PER_DAY = 24UL * 60UL * 60UL;

        /** @brief Seconds since local midnight, false while the clock is not synced. */
        static bool secondsOfDay(uint32_t &seconds);

        /** @brief Seconds since the start time today (or yesterday), 0..SECONDS_PER_DAY-1. */
        uint32_t secondsSinceStart(uint32_t secondsOfDay) const;

        bool _hasEvaluated = false; // false after boot or a change, forces one event
        bool _wasInOnPhase = false;

    // ---------------------------------------------------------------------------
    // Status
    //   statusLine for display and web.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief One short line, changes at most once per minute.
         * @return e.g. `"Timer aus"`, `"8h ab 16:00, aus in 3:12"`, `"8h ab 16:00, ohne Uhrzeit"`.
         */
        String statusLine() const;
    };
}
