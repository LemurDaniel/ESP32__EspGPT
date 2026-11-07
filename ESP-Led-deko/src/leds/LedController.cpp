// NOTE:
//   Implementation of LedController.h.

//
//   Chapters:
//     LIFECYCLE     begin.
//     POWER         togglePower, setPower, isOn and revision.
//     RENDERING     update, pixels and the candle flame.
//     PERSISTENCE   load and save of the NVS state.
//

#include <leds/LedController.h>

namespace Leds
{
    static constexpr const char *PREFS_NAMESPACE = "leds";

    // Candle flame, tuned for maximum light output (red always full, green adds the yellow)
    static constexpr uint8_t FLAME_GREEN_MIN   = 50;  // deep orange
    static constexpr uint8_t FLAME_GREEN_MAX   = 150; // warm yellow
    static constexpr uint8_t FLAME_VALUE_MIN   = 180; // ~70 %, flickers but stays bright
    static constexpr uint8_t FLAME_NOISE_STEP  = 70;  // noise distance between neighbouring LEDs
    static constexpr uint8_t FLAME_SPEED_SHIFT = 2;   // time / 4 => calm, natural flicker
    static constexpr uint8_t GUST_CHANCE       = 2;   // out of 255 per frame, roughly every 2.5 s
    static constexpr uint8_t GUST_MAX          = 60;  // strongest dip of a gust
    static constexpr uint8_t GUST_DECAY        = 4;   // per frame, a gust fades within ~0.3 s

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   1. State    Loads the persisted on / off state.
    //   2. FastLED  Registers strip and onboard LED, brightness and power limit.
    //

    void LedController::begin()
    {
        // State
        load();

        // FastLED
        FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(_strip, LED_COUNT);
        FastLED.addLeds<WS2812B, ONBOARD_LED_PIN, GRB>(_onboard, 1);
        FastLED.setBrightness(LED_BRIGHTNESS);
        // Brightness is scaled down further if the frame would exceed the current budget.
        FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
        FastLED.clear(true);

        Serial.printf("[LedController] %d LEDs on GPIO%d, limit %d mA, flame %s\n",
                      LED_COUNT, LED_DATA_PIN, LED_MAX_MILLIAMPS, _isOn ? "on" : "off");
    }

    // ===========================================================================
    // ================================   POWER   ================================
    // ===========================================================================
    //
    //   togglePower   On to off and back.
    //   setPower      The one real implementation, schedules a save on change.
    //   isOn          Current state.
    //   revision      Change counter for the display.
    //

    void LedController::togglePower()
    {
        setPower(!_isOn);
    }

    void LedController::setPower(bool isOn)
    {
        if (isOn == _isOn)
        {
            return;
        }
        _isOn          = isOn;
        _revision++;
        _isSavePending = true;
        _lastChangeMs  = millis();
        Serial.printf("[LedController] Flame -> %s\n", _isOn ? "on" : "off");
    }

    bool LedController::isOn() const
    {
        return _isOn;
    }

    uint32_t LedController::revision() const
    {
        return _revision;
    }

    // ===========================================================================
    // ==============================   RENDERING   ==============================
    // ===========================================================================
    //
    //   update         Frame pacing, delayed save, off or flame.
    //   pixels         The last frame for the display preview.
    //   renderCandle   The candle flame.
    //
    //   A candle flickers smoothly most of the time and dips briefly when a draught
    //   hits it. So every LED reads its own position from a 2D noise field that moves
    //   over time (smooth flicker), and a rare random gust darkens all LEDs at once
    //   and then fades out. Bright moments shift towards yellow, dim ones towards
    //   deep orange, like a real flame. Red stays at full scale for maximum light.
    //

    void LedController::update()
    {
        const uint32_t now = millis();
        if (now - _lastFrameMs < FRAME_INTERVAL_MS)
        {
            return;
        }
        _lastFrameMs = now;

        if (_isSavePending && now - _lastChangeMs >= SAVE_DELAY_MS)
        {
            _isSavePending = false;
            save();
        }

        if (!_isOn)
        {
            FastLED.clear(true);
            return;
        }

        renderCandle(now);

        // Onboard LED mirrors the first pixel as a status light
        _onboard[0] = _strip[0];
        FastLED.show();
    }

    const CRGB *LedController::pixels() const
    {
        return _strip;
    }

    void LedController::renderCandle(uint32_t now)
    {
        // Gust: rare random start, fades out frame by frame
        if (random8() < GUST_CHANCE)
        {
            _gustLevel = random8(GUST_MAX / 2, GUST_MAX);
        }
        _gustLevel = qsub8(_gustLevel, GUST_DECAY);

        const uint32_t time = now >> FLAME_SPEED_SHIFT;
        for (uint16_t index = 0; index < LED_COUNT; index++)
        {
            // Noise 0..255 => flame value FLAME_VALUE_MIN..255, minus the gust
            const uint8_t noise = inoise8(index * FLAME_NOISE_STEP, time);
            const uint8_t value = qsub8(lerp8by8(FLAME_VALUE_MIN, 255, noise), _gustLevel);
            // Brighter => more yellow, dimmer => more orange.
            // Plain RGB instead of CHSV: CHSV caps the summed channels, full red + green is brighter.
            const uint8_t green = lerp8by8(FLAME_GREEN_MIN, FLAME_GREEN_MAX, noise);
            _strip[index] = CRGB(255, green, 0).nscale8_video(value);
        }
    }

    // ===========================================================================
    // =============================   PERSISTENCE   =============================
    // ===========================================================================
    //
    //   load   Reads the on / off state from NVS, defaults to on.
    //   save   Writes the on / off state to NVS.
    //

    void LedController::load()
    {
        // Read-write, so the namespace exists on the first boot and the read-only error log is skipped
        _prefs.begin(PREFS_NAMESPACE, false);
        _isOn = _prefs.getBool("on", _isOn);
        _prefs.end();
    }

    void LedController::save()
    {
        _prefs.begin(PREFS_NAMESPACE, false);
        _prefs.putBool("on", _isOn);
        _prefs.end();
        Serial.println("[LedController] State saved to NVS");
    }
}
