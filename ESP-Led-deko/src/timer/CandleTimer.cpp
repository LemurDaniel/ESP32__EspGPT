// NOTE:
//   Implementation of CandleTimer.h.

//
//   Chapters:
//     LIFECYCLE   begin.
//     STEPS       cycleStep, hours and save.
//     SCHEDULE    poll and the cycle phase.
//     STATUS      statusLine.
//

#include <esp_timer.h>

#include <timer/CandleTimer.h>

namespace Timers
{
    static constexpr const char *PREFS_NAMESPACE = "timer";
    static constexpr uint64_t MINUTE_MS          = 60ULL * 1000ULL;

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   begin   Loads the stored step, starts the cycle at boot.
    //
    //   Without a real-time clock the boot is the only reference point: plugged in
    //   at 16:00 with an 8 h timer, the candle burns 16:00-24:00 every day.
    //

    void CandleTimer::begin()
    {
        _prefs.begin(PREFS_NAMESPACE, false);
        const uint8_t storedIndex = _prefs.getUChar("step", 0);
        _prefs.end();

        // A step from a newer firmware falls back to off
        _stepIndex     = storedIndex < STEP_COUNT ? storedIndex : 0;
        _cycleStartMs  = nowMs();
        _wasInOnPhase  = false;
        Serial.printf("[CandleTimer] Timer %u h, cycle starts now\n", hours());
    }

    // ===========================================================================
    // ================================   STEPS   ================================
    // ===========================================================================
    //
    //   cycleStep   Next step, restarts the cycle and saves.
    //   hours       Current on duration.
    //   save        Writes the step to NVS.
    //

    uint8_t CandleTimer::cycleStep()
    {
        _stepIndex    = (_stepIndex + 1) % STEP_COUNT;
        _cycleStartMs = nowMs();
        _wasInOnPhase = false;
        save();
        Serial.printf("[CandleTimer] Timer -> %u h\n", hours());
        return hours();
    }

    uint8_t CandleTimer::hours() const
    {
        return STEP_HOURS[_stepIndex];
    }

    void CandleTimer::save()
    {
        // Long presses are rare, so the step is written right away
        _prefs.begin(PREFS_NAMESPACE, false);
        _prefs.putUChar("step", _stepIndex);
        _prefs.end();
    }

    // ===========================================================================
    // ===============================   SCHEDULE   ==============================
    // ===========================================================================
    //
    //   poll          Reports the switch between on and off phase once.
    //   nowMs         64 bit milliseconds since boot.
    //   phaseMs       Position inside the current day.
    //   isInOnPhase   True during the first hours() of each day.
    //
    //   > Timer off
    //       - Report nothing, the button alone decides.
    //   > Phase changed since the last poll
    //       - Report TURN_ON or TURN_OFF.
    //   > Otherwise
    //       - Report nothing, so a manual toggle in between stays until the next switch.
    //

    TimerEvent CandleTimer::poll()
    {
        // Timer off
        if (0 == hours())
        {
            return TimerEvent::NONE;
        }

        // Phase changed since the last poll
        const bool isOnPhase = isInOnPhase();
        if (isOnPhase != _wasInOnPhase)
        {
            _wasInOnPhase = isOnPhase;
            Serial.printf("[CandleTimer] Phase -> %s\n", isOnPhase ? "on" : "off");
            return isOnPhase ? TimerEvent::TURN_ON : TimerEvent::TURN_OFF;
        }

        // Otherwise
        return TimerEvent::NONE;
    }

    uint64_t CandleTimer::nowMs()
    {
        return static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
    }

    uint64_t CandleTimer::phaseMs() const
    {
        return (nowMs() - _cycleStartMs) % DAY_MS;
    }

    bool CandleTimer::isInOnPhase() const
    {
        return phaseMs() < hours() * HOUR_MS;
    }

    // ===========================================================================
    // ================================   STATUS   ===============================
    // ===========================================================================
    //
    //   statusLine   Step and time until the next switch, in hours:minutes.
    //

    String CandleTimer::statusLine() const
    {
        if (0 == hours())
        {
            return "Timer aus";
        }

        // Time until the next switch, rounded up to full minutes so it never shows 0:00 early
        const uint64_t phase        = phaseMs();
        const uint64_t onMs         = hours() * HOUR_MS;
        const bool isOnPhase        = phase < onMs;
        const uint64_t untilSwitch  = isOnPhase ? onMs - phase : DAY_MS - phase;
        const uint32_t minutesTotal = static_cast<uint32_t>((untilSwitch + MINUTE_MS - 1) / MINUTE_MS);

        char line[32];
        snprintf(line, sizeof(line), "Timer %uh  %s in %lu:%02lu", hours(), isOnPhase ? "aus" : "an",
                 static_cast<unsigned long>(minutesTotal / 60), static_cast<unsigned long>(minutesTotal % 60));
        return String(line);
    }
}
