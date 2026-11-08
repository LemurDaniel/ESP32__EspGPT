// NOTE:
//   HTTP API of the candle, mounted by MiniServer::registerRouter().
//   Requires: a CandleBridge that outlives the server and is served by the loop.
//
//   Server->registerRouter(routes_candle::Router(bridge));
//
//   GET  /api/candle   {"power":true,"timerHours":8,"timerStart":"16:00","timerLine":"8h ab 16:00, aus in 3:12",
//                       "clockSynced":true,"clock":"20:48","timerMaxHours":23,"timerPresets":[0,4,8,12]}
//   POST /api/candle   Partial update, every field optional: {"power":false}, {"timerHours":6,"timerStart":"16:00"}
//                      timerHours 0 (off) .. 23, a missing timer field keeps its current value.
//                      Answers 202, the loop applies the request within a few milliseconds.

//
//   Chapters:
//     SERVE THE CANDLE API   The Router class.
//

#pragma once

#include <router/router.h>

#include <web/CandleBridge.h>

namespace routes_candle
{
    // ===========================================================================
    // ========================   SERVE THE CANDLE API   =========================
    // ===========================================================================
    //
    //   Routes     Registers GET and POST on /api/candle.
    //   Handlers   Read the snapshot, validate and forward requests.
    //

    class Router : public EspWeb::Router
    {
    // ---------------------------------------------------------------------------
    // Routes
    //   Registers GET and POST on /api/candle.
    // ---------------------------------------------------------------------------
    public:
        /**
         * @brief Registers the candle routes.
         * @param bridge The bridge the handlers read from and post requests to.
         */
        explicit Router(Web::CandleBridge &bridge);

    // ---------------------------------------------------------------------------
    // Handlers
    //   Read the snapshot, validate and forward requests.
    // ---------------------------------------------------------------------------
    private:
        static void getCandle(Web::CandleBridge &bridge, EspWeb::Request &req, EspWeb::Response &res);
        static void postCandle(Web::CandleBridge &bridge, EspWeb::Request &req, EspWeb::Response &res);
    };
}
