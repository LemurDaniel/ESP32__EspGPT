// NOTE:
//   Implementation of DailyTimer.h.

//
//   Chapters:
//     LIFECYCLE   begin.
//     PRESET      cycleStep and restartCycle.
//     POLLING     poll and statusLine.
//

#include <timer/DailyTimer.h>

#include <esp_timer.h>

namespace Timers
{
    static constexpr const char *PREFS_NAMESPACE = "timer";

    // 0 = off
    static constexpr uint8_t PRESET_HOURS[] = {0, 4, 8, 12};
    static constexpr size_t PRESET_COUNT    = sizeof(PRESET_HOURS) / sizeof(PRESET_HOURS[0]);

    static constexpr int64_t MICROS_PER_SECOND  = 1000000;
    static constexpr uint32_t SECONDS_PER_HOUR  = 3600;
    static constexpr uint32_t SECONDS_PER_DAY   = 24 * SECONDS_PER_HOUR;
    static constexpr uint32_t SECONDS_PER_MINUTE = 60;

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================

    void DailyTimer::begin()
    {
        // Read-write, so the namespace exists on the first boot and the read-only error log is skipped
        _prefs.begin(PREFS_NAMESPACE, false);
        _hours = _prefs.getUChar("hours", 0);
        _prefs.end();

        restartCycle();
        Serial.printf("[DailyTimer] Preset %u h\n", _hours);
    }

    // ===========================================================================
    // ===============================   PRESET   ================================
    // ===========================================================================
    //
    //   cycleStep      Next preset, saved right away (a tap is rare, unlike a slider drag).
    //   restartCycle   The on time starts now.
    //

    void DailyTimer::cycleStep()
    {
        // Position of the current preset, an unknown stored value restarts at off
        size_t index = 0;
        for (size_t candidate = 0; candidate < PRESET_COUNT; candidate++)
        {
            if (PRESET_HOURS[candidate] == _hours)
            {
                index = candidate;
            }
        }
        _hours = PRESET_HOURS[(index + 1) % PRESET_COUNT];

        _prefs.begin(PREFS_NAMESPACE, false);
        _prefs.putUChar("hours", _hours);
        _prefs.end();

        restartCycle();
        Serial.printf("[DailyTimer] Preset -> %u h\n", _hours);
    }

    void DailyTimer::restartCycle()
    {
        // 64 bit microseconds: millis() would wrap after 49 days
        _cycleStartUs     = esp_timer_get_time();
        _isOnPhase        = false;
        _remainingMinutes = 0;
        _lastPollMs       = millis() - POLL_INTERVAL_MS; // the next poll runs right away
        _revision++;
    }

    // ===========================================================================
    // ===============================   POLLING   ===============================
    // ===========================================================================
    //
    //   One day of the cycle with a 12 h preset:
    //
    //     0 h          12 h                   24 h
    //     |--- on -----|-------- off ---------|--- on ---  ...
    //     TURN_ON      TURN_OFF               TURN_ON
    //
    //   Events fire only at the switches. Toggling the power by hand in between
    //   holds until the next switch.
    //

    TimerEvent DailyTimer::poll()
    {
        if (0 == _hours)
        {
            return TimerEvent::NONE;
        }
        const uint32_t now = millis();
        if (now - _lastPollMs < POLL_INTERVAL_MS)
        {
            return TimerEvent::NONE;
        }
        _lastPollMs = now;

        // Position inside the current day of the cycle
        const int64_t elapsedSeconds = (esp_timer_get_time() - _cycleStartUs) / MICROS_PER_SECOND;
        const uint32_t daySeconds    = static_cast<uint32_t>(elapsedSeconds % SECONDS_PER_DAY);
        const uint32_t onSeconds     = _hours * SECONDS_PER_HOUR;
        const bool isOnPhase         = daySeconds < onSeconds;

        // Remaining time until the next switch, rounded up so it never shows 0:00
        const uint32_t remainingSeconds = isOnPhase ? onSeconds - daySeconds : SECONDS_PER_DAY - daySeconds;
        const uint32_t remainingMinutes = (remainingSeconds + SECONDS_PER_MINUTE - 1) / SECONDS_PER_MINUTE;
        if (remainingMinutes != _remainingMinutes)
        {
            _remainingMinutes = remainingMinutes;
            _revision++;
        }

        if (isOnPhase == _isOnPhase)
        {
            return TimerEvent::NONE;
        }
        _isOnPhase = isOnPhase;
        _revision++;
        Serial.printf("[DailyTimer] Switch -> %s\n", _isOnPhase ? "on" : "off");
        return _isOnPhase ? TimerEvent::TURN_ON : TimerEvent::TURN_OFF;
    }

    void DailyTimer::statusLine(char *buffer, size_t size) const
    {
        if (0 == _hours)
        {
            snprintf(buffer, size, "%s", "");
            return;
        }
        // 192 minutes => "aus in 3:12"
        snprintf(buffer, size, "%s in %lu:%02lu", _isOnPhase ? "aus" : "an",
                 static_cast<unsigned long>(_remainingMinutes / 60),
                 static_cast<unsigned long>(_remainingMinutes % 60));
    }
}
