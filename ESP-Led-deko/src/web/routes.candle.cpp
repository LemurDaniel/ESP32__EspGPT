// NOTE:
//   Implementation of routes.candle.h.

//
//   Chapters:
//     HELPERS                Time parsing and formatting, the snapshot as JSON.
//     SERVE THE CANDLE API   Route registration and the handlers.
//

#include <timer/CandleTimer.h>
#include <web/routes.candle.h>

namespace routes_candle
{
    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   formatTime      Minute of the day to "HH:MM".
    //   parseTime       "HH:MM" to minute of the day, rejects any other format.
    //   writeSnapshot   The snapshot plus timer presets and maximum.
    //

    static void formatTime(uint16_t minutes, char (&buffer)[6])
    {
        snprintf(buffer, sizeof(buffer), "%02u:%02u", minutes / 60, minutes % 60);
    }

    static bool parseTime(const char *text, uint16_t &minutes)
    {
        // "16:30" => 990
        if (nullptr == text || 5 != strlen(text) || ':' != text[2]
            || !isdigit(text[0]) || !isdigit(text[1]) || !isdigit(text[3]) || !isdigit(text[4]))
        {
            return false;
        }
        const uint16_t hour   = (text[0] - '0') * 10 + (text[1] - '0');
        const uint16_t minute = (text[3] - '0') * 10 + (text[4] - '0');
        if (hour > 23 || minute > 59)
        {
            return false;
        }
        minutes = hour * 60 + minute;
        return true;
    }

    static void writeSnapshot(const Web::CandleSnapshot &snapshot, JsonDocument &doc)
    {
        char start[6];
        formatTime(snapshot.timerStart, start);

        doc["power"]       = snapshot.isOn;
        doc["timerHours"]  = snapshot.timerHours;
        doc["timerStart"]  = start;
        doc["timerLine"]   = snapshot.timerLine;
        doc["clockSynced"] = snapshot.isClockSynced;
        doc["clock"]       = snapshot.clock;

        doc["timerMaxHours"] = Timers::CandleTimer::MAX_HOURS;
        JsonArray presets    = doc["timerPresets"].to<JsonArray>();
        for (const uint8_t hours : Timers::CandleTimer::PRESET_HOURS)
        {
            presets.add(hours);
        }
    }

    // ===========================================================================
    // ========================   SERVE THE CANDLE API   =========================
    // ===========================================================================
    //
    //   Router       Registers the routes, binding the bridge into each handler.
    //   getCandle    Returns the last published state.
    //   postCandle   Validates every given field, forwards them together or none.
    //

    Router::Router(Web::CandleBridge &bridge)
    {
        Web::CandleBridge *candle = &bridge;
        route("GET", "/api/candle", [candle](EspWeb::Request &req, EspWeb::Response &res)
        {
            getCandle(*candle, req, res);
        });
        route("POST", "/api/candle", [candle](EspWeb::Request &req, EspWeb::Response &res)
        {
            postCandle(*candle, req, res);
        });
    }

    void Router::getCandle(Web::CandleBridge &bridge, EspWeb::Request &req, EspWeb::Response &res)
    {
        JsonDocument doc;
        writeSnapshot(bridge.snapshot(), doc);
        res.json(doc).OK();
    }

    void Router::postCandle(Web::CandleBridge &bridge, EspWeb::Request &req, EspWeb::Response &res)
    {
        const JsonDocument &body       = req.body.json();
        const JsonVariantConst power   = body["power"];
        const JsonVariantConst hours   = body["timerHours"];
        const JsonVariantConst start   = body["timerStart"];
        const Web::CandleSnapshot current = bridge.snapshot();

        // A missing timer field keeps its current value
        uint8_t timerHours  = current.timerHours;
        uint16_t timerStart = current.timerStart;

        // Validations
        if (power.isNull() && hours.isNull() && start.isNull())
        {
            res.text("Body must contain at least one of: power, timerHours, timerStart.").BadRequest();
            return;
        }
        if (!power.isNull() && !power.is<bool>())
        {
            res.text("power must be true or false.").BadRequest();
            return;
        }
        if (!hours.isNull())
        {
            if (!hours.is<int>() || hours.as<int>() < 0 || hours.as<int>() > 255
                || !Timers::CandleTimer::isValidHours(static_cast<uint8_t>(hours.as<int>())))
            {
                res.text("timerHours must be an integer between 0 (off) and 23.").BadRequest();
                return;
            }
            timerHours = static_cast<uint8_t>(hours.as<int>());
        }
        if (!start.isNull() && !parseTime(start.as<const char *>(), timerStart))
        {
            res.text("timerStart must be a time in the format 'HH:MM', e.g. '16:00'.").BadRequest();
            return;
        }

        // Forward. Timer first: it applies its plan, an explicit power in the same request wins.
        if (!hours.isNull() || !start.isNull())
        {
            bridge.requestTimer(timerHours, timerStart);
        }
        if (!power.isNull())
        {
            bridge.requestPower(power.as<bool>());
        }
        Serial.println("[routes_candle] Request forwarded to the loop");
        res.text("Accepted").status(202);
    }
}
