// NOTE:
//   Entry point: candle flame on the WS2812B LEDs, daily wall-clock timer, web UI, status
//   display and BOOT button on the Waveshare ESP32-C6-LCD-1.47.
//   Requires: include/secrets.h (template: include/secrets.example.h) and the web UI
//   uploaded via uploadfs.
//
//   pio run -e waveshare-c6-lcd147 --target upload
//   pio run -e waveshare-c6-lcd147 --target uploadfs
//
//   BOOT button: short press = flame on / off, long press = timer preset off -> 4 -> 8 -> 12 h.
//   Web UI:      http://led-deko.local

//
//   Chapters:
//     START THE DEVICE   setup, LEDs first so the flame lights up before WiFi is ready.
//     RUN THE LOOP       loop, inputs, timer, LED frame, display and the web snapshot.
//

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <MiniServer.h>

#include <secrets.h>

#include <display/StatusDisplay.h>
#include <input/BootButton.h>
#include <leds/LedController.h>
#include <timer/CandleTimer.h>
#include <web/CandleBridge.h>
#include <web/routes.candle.h>

static constexpr const char *DNS_NAME     = "led-deko"; // reachable as led-deko.local
static constexpr const char *TIMEZONE     = "CET-1CEST,M3.5.0,M10.5.0/3"; // Europe/Berlin incl. daylight saving
static constexpr const char *NTP_SERVER_1 = "pool.ntp.org";
static constexpr const char *NTP_SERVER_2 = "time.cloudflare.com";
// One worker only: MiniServer pins worker i to core (i % 2), the single-core C6 has no core 1.
static constexpr int SERVER_WORKER_COUNT = 1;

// Created in setup(): the MiniServer constructor mounts LittleFS, which as a global
// would run before the core's LittleFS object exists and crash at boot.
EspWeb::MiniServer *Server = nullptr;

Leds::LedController LedStrip;
Display::StatusDisplay Screen;
Input::BootButton Button(BOOT_BUTTON_PIN);
Timers::CandleTimer Timer;
Web::CandleBridge Bridge;

// ===========================================================================
// ==========================   START THE DEVICE   ===========================
// ===========================================================================
//
//   1. Hardware   Serial, LEDs, display, button and timer.
//   2. Server     Credentials from secrets.h, routes, web UI and HTTP server.
//   3. Clock      SNTP with the Berlin time zone, syncs once WiFi is up.
//

void setup()
{
    // ---------------------------------------------------------------------------
    // Hardware
    //   Serial, LEDs, display, button and timer.
    // ---------------------------------------------------------------------------

    Serial.begin(115200);
    LedStrip.begin();
    Screen.begin();
    Button.begin();
    Timer.begin();

    // ---------------------------------------------------------------------------
    // Server
    //   Credentials from secrets.h, routes, web UI and HTTP server.
    // ---------------------------------------------------------------------------

    Server = new EspWeb::MiniServer();

    // Placeholders from secrets.example.h are skipped, the setup access point takes over
    if (0 != strcmp(WIFI_SSID, "<SSID>") && 0 != strcmp(WIFI_PASSWORD, "<WIFI_PASSWORD>"))
    {
        Server->connectWiFi(WIFI_SSID, WIFI_PASSWORD);
    }
    if (0 != strcmp(ADMIN_PASSWORD, "<ADMIN_PASSWORD>"))
    {
        Server->defaultAdminCredentials(ADMIN_USER, ADMIN_PASSWORD);
    }

    Server->registerRouter(routes_candle::Router(Bridge));
    Server->index("/web/index.html");
    Server->dns(DNS_NAME);
    Server->start(80, "0.0.0.0", SERVER_WORKER_COUNT);

    // ---------------------------------------------------------------------------
    // Clock
    //   SNTP with the Berlin time zone, syncs once WiFi is up.
    // ---------------------------------------------------------------------------

    configTzTime(TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);
}

// ===========================================================================
// ============================   RUN THE LOOP   =============================
// ===========================================================================
//
//   1. Button     Short press = flame on / off, long press = next timer preset.
//   2. Timer      Applies web timer settings, then switches at the burn time edges.
//   3. Web power  Applies a web power request, after the timer so it wins.
//   4. Render     One LED frame, paced inside LedController.
//   5. Display    Page on change, timer line, live preview of the frame.
//   6. Snapshot   Publishes the state for the web handlers.
//
//   Everything that touches LedController or CandleTimer runs here in the loop task,
//   the web handlers only talk to the CandleBridge.
//

void loop()
{
    // ---------------------------------------------------------------------------
    // Button
    //   Short press = flame on / off, long press = next timer preset.
    // ---------------------------------------------------------------------------

    switch (Button.poll())
    {
    case Input::ButtonEvent::SHORT_PRESS: LedStrip.togglePower(); break;
    case Input::ButtonEvent::LONG_PRESS:  Timer.cycleStep();      break;
    default:                              break;
    }

    // ---------------------------------------------------------------------------
    // Timer
    //   Applies web timer settings, then switches at the burn time edges.
    // ---------------------------------------------------------------------------

    uint8_t requestedHours  = 0;
    uint16_t requestedStart = 0;
    if (Bridge.takeTimerRequest(requestedHours, requestedStart))
    {
        Timer.configure(requestedHours, requestedStart);
    }

    switch (Timer.poll())
    {
    case Timers::TimerEvent::TURN_ON:  LedStrip.setPower(true);  break;
    case Timers::TimerEvent::TURN_OFF: LedStrip.setPower(false); break;
    default:                           break;
    }

    // ---------------------------------------------------------------------------
    // Web power
    //   Applies a web power request, after the timer so it wins.
    // ---------------------------------------------------------------------------

    bool requestedIsOn = false;
    if (Bridge.takePowerRequest(requestedIsOn))
    {
        LedStrip.setPower(requestedIsOn);
    }

    // ---------------------------------------------------------------------------
    // Render
    //   One LED frame, paced inside LedController.
    // ---------------------------------------------------------------------------

    LedStrip.update();

    // ---------------------------------------------------------------------------
    // Display
    //   Page on change, timer line, live preview of the frame.
    // ---------------------------------------------------------------------------

    const String timerLine = Timer.statusLine();
    Screen.update(LedStrip.isOn(), LedStrip.revision(), LedStrip.pixels(), timerLine);

    // ---------------------------------------------------------------------------
    // Snapshot
    //   Publishes the state for the web handlers.
    // ---------------------------------------------------------------------------

    Web::CandleSnapshot snapshot;
    snapshot.isOn          = LedStrip.isOn();
    snapshot.timerHours    = Timer.hours();
    snapshot.timerStart    = Timer.startMinutes();
    snapshot.isClockSynced = Timers::CandleTimer::isClockSynced();
    if (snapshot.isClockSynced)
    {
        const time_t now = time(nullptr);
        struct tm local;
        localtime_r(&now, &local);
        strftime(snapshot.clock, sizeof(snapshot.clock), "%H:%M", &local);
    }
    strlcpy(snapshot.timerLine, timerLine.c_str(), sizeof(snapshot.timerLine));
    Bridge.publish(snapshot);

    delay(1);
}
