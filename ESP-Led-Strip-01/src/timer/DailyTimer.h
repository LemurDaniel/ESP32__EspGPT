// NOTE:
//   Daily on / off timer without a real-time clock: on for N hours, off for the rest
//   of the day, on again 24 h after it was set. The preset is persisted in NVS.
//   Requires: only use it from the loop task, it has no locking.
//
//   Timers::DailyTimer timer;
//   timer.begin();                          // in setup()
//   timer.cycleStep();                      // from the UI: off -> 4 -> 8 -> 12 h -> off
//   switch (timer.poll()) { ... }           // in loop(), TURN_ON / TURN_OFF at the switches

//
//   Chapters:
//     SWITCH BY THE DAY   The TimerEvent enum and the DailyTimer class.
//

#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace Timers
{
    // ===========================================================================
    // ==========================   SWITCH BY THE DAY   ==========================
    // ===========================================================================
    //
    //   Lifecycle   begin, loads the preset and starts the cycle.
    //   Preset      cycleStep and hours, the on time per day.
    //   Polling     poll, statusLine and revision.
    //
    //   Without a clock the moment of setting is the reference: the cycle starts
    //   when the preset is chosen, or at boot after a power cut.
    //

    /** @brief What the strip should do now, fired once at each switch of the cycle. */
    enum class TimerEvent : uint8_t
    {
        NONE,
        TURN_ON,
        TURN_OFF,
    };

    class DailyTimer
    {
    // ---------------------------------------------------------------------------
    // Lifecycle
    //   begin, loads the preset and starts the cycle.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Loads the persisted preset, an active timer starts its on time now. */
        void begin();

    // ---------------------------------------------------------------------------
    // Preset
    //   cycleStep and hours, the on time per day.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Selects the next preset (off -> 4 -> 8 -> 12 h -> off) and restarts the cycle. */
        void cycleStep();

        /** @brief On time per day in hours, 0 while the timer is off. */
        uint8_t hours() const { return _hours; }

    private:
        void restartCycle();

        Preferences _prefs;
        uint8_t _hours        = 0;
        int64_t _cycleStartUs = 0;

    // ---------------------------------------------------------------------------
    // Polling
    //   poll, statusLine and revision.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Checks the cycle (call from loop()).
         * @return TURN_ON or TURN_OFF once when the phase switches, otherwise NONE.
         */
        TimerEvent poll();

        /**
         * @brief Writes the time until the next switch, e.g. `aus in 3:12` or `an in 11:02`.
         * @param buffer Target, left empty while the timer is off.
         * @param size   Size of the target in bytes.
         */
        void statusLine(char *buffer, size_t size) const;

        /** @brief Increments when preset, phase or the remaining minutes change. */
        uint32_t revision() const { return _revision; }

    private:
        static constexpr uint32_t POLL_INTERVAL_MS = 250;

        bool _isOnPhase            = false; // false after a restart, so the first poll fires TURN_ON
        uint32_t _remainingMinutes = 0;     // until the next switch, rounded up
        uint32_t _lastPollMs       = 0;
        uint32_t _revision         = 0;
    };
}
