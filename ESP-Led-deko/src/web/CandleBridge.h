// NOTE:
//   Hands state and commands between the loop task and the web server worker task.
//   LedController and CandleTimer are not thread safe, so the web handlers never touch
//   them: they read the snapshot the loop publishes and leave requests the loop applies.
//
//   Web::CandleBridge bridge;
//   bridge.publish(snapshot);                  // loop: after every pass
//   bridge.requestPower(true);                 // web handler
//   if (bridge.takePowerRequest(isOn)) ...     // loop: applies the request

//
//   Chapters:
//     BRIDGE LOOP AND WEB   CandleSnapshot and the CandleBridge class.
//

#pragma once

#include <Arduino.h>

#include <freertos/FreeRTOS.h>

namespace Web
{
    // ===========================================================================
    // =========================   BRIDGE LOOP AND WEB   =========================
    // ===========================================================================
    //
    //   Snapshot   The state the web UI shows.
    //   State      publish and snapshot, loop writes, web reads.
    //   Requests   request* from the web, take* in the loop, the latest wins.
    //

    // ---------------------------------------------------------------------------
    // Snapshot
    //   The state the web UI shows.
    // ---------------------------------------------------------------------------

    /** @brief Copy of the candle state, published by the loop. */
    struct CandleSnapshot
    {
        bool isOn              = false;
        uint8_t timerHours     = 0;
        uint16_t timerStart    = 0;  // minute of the day
        bool isClockSynced     = false;
        char clock[6]          = ""; // local time "HH:MM", empty while not synced
        char timerLine[48]     = ""; // CandleTimer::statusLine(), e.g. "8h ab 16:00, aus in 3:12"
    };

    class CandleBridge
    {
    // ---------------------------------------------------------------------------
    // State
    //   publish and snapshot, loop writes, web reads.
    // ---------------------------------------------------------------------------
    public:
        /** @brief Stores the current state (loop task). */
        void publish(const CandleSnapshot &snapshot);

        /** @brief Returns a copy of the last published state (web task). */
        CandleSnapshot snapshot();

    private:
        // A spinlock: held only for a struct copy, never across a blocking call.
        portMUX_TYPE _lock = portMUX_INITIALIZER_UNLOCKED;
        CandleSnapshot _snapshot;

    // ---------------------------------------------------------------------------
    // Requests
    //   request* from the web, take* in the loop, the latest wins.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Asks the loop to switch the flame (web task).
         * @param isOn True to light the flame.
         */
        void requestPower(bool isOn);

        /**
         * @brief Asks the loop to configure the timer (web task).
         * @param hours        0, 4, 8 or 12, validated by the caller.
         * @param startMinutes Minute of the day, validated by the caller.
         */
        void requestTimer(uint8_t hours, uint16_t startMinutes);

        /**
         * @brief Takes a pending power request (loop task).
         * @param isOn Receives the requested state.
         * @return True when a request was pending, it is cleared.
         */
        bool takePowerRequest(bool &isOn);

        /**
         * @brief Takes a pending timer request (loop task).
         * @param hours        Receives the requested hours.
         * @param startMinutes Receives the requested start time.
         * @return True when a request was pending, it is cleared.
         */
        bool takeTimerRequest(uint8_t &hours, uint16_t &startMinutes);

    private:
        bool _hasPowerRequest   = false;
        bool _requestedIsOn     = false;
        bool _hasTimerRequest   = false;
        uint8_t _requestedHours = 0;
        uint16_t _requestedStart = 0;
    };
}
