// NOTE:
//   Implementation of CandleBridge.h.

//
//   Chapters:
//     STATE      publish and snapshot.
//     REQUESTS   request* and take*.
//

#include <web/CandleBridge.h>

namespace Web
{
    // ===========================================================================
    // ================================   STATE   ================================
    // ===========================================================================
    //
    //   publish    Copies the state in under the lock.
    //   snapshot   Copies the state out under the lock.
    //

    void CandleBridge::publish(const CandleSnapshot &snapshot)
    {
        portENTER_CRITICAL(&_lock);
        _snapshot = snapshot;
        portEXIT_CRITICAL(&_lock);
    }

    CandleSnapshot CandleBridge::snapshot()
    {
        portENTER_CRITICAL(&_lock);
        const CandleSnapshot copy = _snapshot;
        portEXIT_CRITICAL(&_lock);
        return copy;
    }

    // ===========================================================================
    // ===============================   REQUESTS   ==============================
    // ===========================================================================
    //
    //   requestPower        Stores a power request, overwrites an older one.
    //   requestTimer        Stores a timer request, overwrites an older one.
    //   takePowerRequest    Hands a pending power request to the loop.
    //   takeTimerRequest    Hands a pending timer request to the loop.
    //

    void CandleBridge::requestPower(bool isOn)
    {
        portENTER_CRITICAL(&_lock);
        _requestedIsOn   = isOn;
        _hasPowerRequest = true;
        portEXIT_CRITICAL(&_lock);
    }

    void CandleBridge::requestTimer(uint8_t hours, uint16_t startMinutes)
    {
        portENTER_CRITICAL(&_lock);
        _requestedHours  = hours;
        _requestedStart  = startMinutes;
        _hasTimerRequest = true;
        portEXIT_CRITICAL(&_lock);
    }

    bool CandleBridge::takePowerRequest(bool &isOn)
    {
        portENTER_CRITICAL(&_lock);
        const bool hasRequest = _hasPowerRequest;
        isOn                  = _requestedIsOn;
        _hasPowerRequest      = false;
        portEXIT_CRITICAL(&_lock);
        return hasRequest;
    }

    bool CandleBridge::takeTimerRequest(uint8_t &hours, uint16_t &startMinutes)
    {
        portENTER_CRITICAL(&_lock);
        const bool hasRequest = _hasTimerRequest;
        hours                 = _requestedHours;
        startMinutes          = _requestedStart;
        _hasTimerRequest      = false;
        portEXIT_CRITICAL(&_lock);
        return hasRequest;
    }
}
