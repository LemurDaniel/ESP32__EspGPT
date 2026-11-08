// NOTE:
//   Implementation of CandleTimer.h.

//
//   Chapters:
//     LIFECYCLE   begin.
//     SETTINGS    cycleStep, configure, the getters, apply and save.
//     SCHEDULE    poll, the clock and the phase of the day.
//     STATUS      statusLine.
//

#include <time.h>

#include <timer/CandleTimer.h>

namespace Timers
{
    static constexpr const char *PREFS_NAMESPACE = "timer";
    static constexpr time_t SYNCED_AFTER_EPOCH  = 1700000000; // 2023-11-14, anything earlier is the unsynced 1970 clock

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   begin   Loads duration and start time, falls back to off / 17:00.
    //

    void CandleTimer::begin()
    {
        _prefs.begin(PREFS_NAMESPACE, false);
        const uint8_t storedHours  = _prefs.getUChar("hours", 0);
        const uint16_t storedStart = _prefs.getUShort("start", DEFAULT_START_MINUTES);
        _prefs.end();

        // Values from a newer firmware fall back to the defaults
        _hours        = isValidHours(storedHours) ? storedHours : 0;
        _startMinutes = storedStart < MINUTES_PER_DAY ? storedStart : DEFAULT_START_MINUTES;
        _hasEvaluated = false;
        Serial.printf("[CandleTimer] Timer %u h from %02u:%02u\n", hours(), _startMinutes / 60, _startMinutes % 60);
    }

    // ===========================================================================
    // ===============================   SETTINGS   ==============================
    // ===========================================================================
    //
    //   cycleStep      Next preset, start = now when the clock is synced.
    //   configure      Duration and start time together.
    //   isValidHours   0..MAX_HOURS.
    //   hours          Current burn duration.
    //   startMinutes   Current start time.
    //   apply          Stores, saves and forces a fresh evaluation.
    //   save           Writes duration and start time to NVS.
    //

    uint8_t CandleTimer::cycleStep()
    {
        // Next preset above the current duration, 0 after the last one.
        // A web value in between (e.g. 6 h) continues with the next preset (8 h).
        uint8_t nextHours = 0;
        for (const uint8_t preset : PRESET_HOURS)
        {
            if (preset > _hours)
            {
                nextHours = preset;
                break;
            }
        }

        // Without a synced clock the stored start time stays
        uint16_t startMinutes = _startMinutes;
        uint32_t seconds      = 0;
        if (secondsOfDay(seconds))
        {
            startMinutes = static_cast<uint16_t>(seconds / 60);
        }
        apply(nextHours, startMinutes);
        return _hours;
    }

    bool CandleTimer::configure(uint8_t burnHours, uint16_t startMinutes)
    {
        if (!isValidHours(burnHours) || startMinutes >= MINUTES_PER_DAY)
        {
            return false;
        }
        apply(burnHours, startMinutes);
        return true;
    }

    bool CandleTimer::isValidHours(uint8_t hours)
    {
        return hours <= MAX_HOURS;
    }

    uint8_t CandleTimer::hours() const
    {
        return _hours;
    }

    uint16_t CandleTimer::startMinutes() const
    {
        return _startMinutes;
    }

    void CandleTimer::apply(uint8_t burnHours, uint16_t startMinutes)
    {
        _hours        = burnHours;
        _startMinutes = startMinutes;
        _hasEvaluated = false; // the next poll applies the plan right away
        save();
        Serial.printf("[CandleTimer] Timer -> %u h from %02u:%02u\n", hours(), _startMinutes / 60, _startMinutes % 60);
    }

    void CandleTimer::save()
    {
        // Changes are rare (button or web), so they are written right away
        _prefs.begin(PREFS_NAMESPACE, false);
        _prefs.putUChar("hours", _hours);
        _prefs.putUShort("start", _startMinutes);
        _prefs.end();
    }

    // ===========================================================================
    // ===============================   SCHEDULE   ==============================
    // ===========================================================================
    //
    //   poll                Reports the phase once after boot / change, then each switch.
    //   isClockSynced       SNTP delivered a plausible time.
    //   secondsOfDay        Local seconds since midnight.
    //   secondsSinceStart   Distance to the start time, wraps over midnight.
    //
    //   > Timer off or clock not synced
    //       - Report nothing, the button alone decides.
    //   > First evaluation after boot or a change
    //       - Report the current phase, so the plan applies right away.
    //   > Phase changed since the last poll
    //       - Report TURN_ON or TURN_OFF.
    //   > Otherwise
    //       - Report nothing, so a manual toggle in between stays until the next switch.
    //

    TimerEvent CandleTimer::poll()
    {
        // Timer off or clock not synced
        uint32_t seconds = 0;
        if (0 == hours() || !secondsOfDay(seconds))
        {
            return TimerEvent::NONE;
        }

        const bool isOnPhase = secondsSinceStart(seconds) < static_cast<uint32_t>(hours()) * 3600UL;

        // First evaluation after boot or a change
        // Phase changed since the last poll
        if (!_hasEvaluated || isOnPhase != _wasInOnPhase)
        {
            _hasEvaluated = true;
            _wasInOnPhase = isOnPhase;
            Serial.printf("[CandleTimer] Phase -> %s\n", isOnPhase ? "on" : "off");
            return isOnPhase ? TimerEvent::TURN_ON : TimerEvent::TURN_OFF;
        }

        // Otherwise
        return TimerEvent::NONE;
    }

    bool CandleTimer::isClockSynced()
    {
        return time(nullptr) > SYNCED_AFTER_EPOCH;
    }

    bool CandleTimer::secondsOfDay(uint32_t &seconds)
    {
        if (!isClockSynced())
        {
            return false;
        }
        const time_t now = time(nullptr);
        struct tm local;
        localtime_r(&now, &local);
        seconds = static_cast<uint32_t>(local.tm_hour) * 3600UL + local.tm_min * 60UL + local.tm_sec;
        return true;
    }

    uint32_t CandleTimer::secondsSinceStart(uint32_t secondsOfDay) const
    {
        // 23:00 with start 16:00 => 7 h, 02:00 with start 16:00 => 10 h (wrapped over midnight)
        const uint32_t startSeconds = static_cast<uint32_t>(_startMinutes) * 60UL;
        return (secondsOfDay + SECONDS_PER_DAY - startSeconds) % SECONDS_PER_DAY;
    }

    // ===========================================================================
    // ================================   STATUS   ===============================
    // ===========================================================================
    //
    //   statusLine   Duration, start and time until the next switch in hours:minutes.
    //

    String CandleTimer::statusLine() const
    {
        if (0 == hours())
        {
            return "Timer aus";
        }

        char prefix[16];
        snprintf(prefix, sizeof(prefix), "%uh ab %02u:%02u", hours(), _startMinutes / 60, _startMinutes % 60);

        uint32_t seconds = 0;
        if (!secondsOfDay(seconds))
        {
            return String(prefix) + ", ohne Uhrzeit";
        }

        // Time until the next switch, rounded up to full minutes so it never shows 0:00 early
        const uint32_t sinceStart   = secondsSinceStart(seconds);
        const uint32_t burnSeconds  = static_cast<uint32_t>(hours()) * 3600UL;
        const bool isOnPhase        = sinceStart < burnSeconds;
        const uint32_t untilSwitch  = isOnPhase ? burnSeconds - sinceStart : SECONDS_PER_DAY - sinceStart;
        const uint32_t minutesTotal = (untilSwitch + 59) / 60;

        char line[40];
        snprintf(line, sizeof(line), "%s, %s in %lu:%02lu", prefix, isOnPhase ? "aus" : "an",
                 static_cast<unsigned long>(minutesTotal / 60), static_cast<unsigned long>(minutesTotal % 60));
        return String(line);
    }
}
