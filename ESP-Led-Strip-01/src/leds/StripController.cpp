// NOTE:
//   Implementation of StripController.h.

//
//   Chapters:
//     LIFECYCLE     begin.
//     SETTINGS      Power, cycleMode, the colour bar mapping and markChanged.
//     RENDERING     update, the effects and the candle flame.
//     PERSISTENCE   load and save of the NVS settings.
//

#include <leds/StripController.h>

namespace Leds
{
    static constexpr const char *PREFS_NAMESPACE = "strip";
    static constexpr uint8_t STRIP_VOLTS         = 5;

    // Colour bar: two white fields at the left end, then the colour wheel
    static constexpr uint8_t WARM_WHITE_END = 16; // positions below are warm white
    static constexpr uint8_t WHITE_END      = 32; // positions below are white
    static const CRGB WARM_WHITE            = CRGB(255, 150, 50);

    // Effects
    static constexpr uint8_t RAINBOW_HUE_STEP  = 3;  // hue distance between neighbouring LEDs
    static constexpr uint8_t BREATHE_BPM       = 12; // one breath every 5 s
    static constexpr uint8_t BREATHE_VALUE_MIN = 30; // never fully dark

    // Candle flame, same tuning as ESP-Led-deko (red always full, green adds the yellow)
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

    void StripController::begin()
    {
        load();

        FastLED.addLeds<WS2812, LED_DATA_PIN, GRB>(_strip, LED_COUNT);
        FastLED.setBrightness(LED_BRIGHTNESS);
        // Brightness is scaled down further if the frame would exceed the current budget.
        FastLED.setMaxPowerInVoltsAndMilliamps(STRIP_VOLTS, LED_MAX_MILLIAMPS);
        FastLED.clear(true);

        Serial.printf("[StripController] %d LEDs on GPIO%d, limit %d mA, %s, mode %d, colour position %d\n",
                      LED_COUNT, LED_DATA_PIN, LED_MAX_MILLIAMPS, _isOn ? "on" : "off",
                      static_cast<int>(_mode), _colorPosition);
    }

    // ===========================================================================
    // ==============================   SETTINGS   ===============================
    // ===========================================================================
    //
    //   togglePower        On to off and back, delegates to setPower.
    //   setPower           Ignores an unchanged state, so the timer may repeat itself.
    //   cycleMode          Next effect.
    //   setColorPosition   Colour from a position on the colour bar.
    //   colorAt            The one mapping from bar position to colour.
    //   markChanged        The one place that bumps the revision and schedules the save.
    //

    void StripController::togglePower()
    {
        setPower(!_isOn);
    }

    void StripController::setPower(bool isOn)
    {
        if (isOn == _isOn)
        {
            return;
        }
        _isOn = isOn;
        markChanged();
        Serial.printf("[StripController] Power -> %s\n", _isOn ? "on" : "off");
    }

    void StripController::cycleMode()
    {
        _mode = static_cast<StripMode>((static_cast<uint8_t>(_mode) + 1) % STRIP_MODE_COUNT);
        markChanged();
        Serial.printf("[StripController] Mode -> %d\n", static_cast<int>(_mode));
    }

    void StripController::setColorPosition(uint8_t position)
    {
        // Unchanged while the finger rests: neither redraw nor save
        if (position == _colorPosition)
        {
            return;
        }
        _colorPosition = position;
        markChanged();
    }

    CRGB StripController::colorAt(uint8_t position)
    {
        if (position < WARM_WHITE_END)
        {
            return WARM_WHITE;
        }
        if (position < WHITE_END)
        {
            return CRGB::White;
        }
        // 32..255 => hue 0..255
        const uint8_t hue = (position - WHITE_END) * 255 / (255 - WHITE_END);
        return CHSV(hue, 255, 255);
    }

    void StripController::markChanged()
    {
        _revision++;
        _isSavePending = true;
        _lastChangeMs  = millis();
    }

    // ===========================================================================
    // ==============================   RENDERING   ==============================
    // ===========================================================================

    void StripController::update()
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

        switch (_mode)
        {
        case StripMode::SOLID:
            fill_solid(_strip, LED_COUNT, color());
            break;

        case StripMode::CANDLE:
            renderCandle(now);
            break;

        case StripMode::BREATHE:
        {
            CRGB breath = color();
            breath.nscale8_video(beatsin8(BREATHE_BPM, BREATHE_VALUE_MIN, 255));
            fill_solid(_strip, LED_COUNT, breath);
            break;
        }

        case StripMode::RAINBOW:
            fill_rainbow(_strip, LED_COUNT, _rainbowHue, RAINBOW_HUE_STEP);
            _rainbowHue++;
            break;
        }

        FastLED.show();
    }

    // A candle flickers smoothly most of the time and dips briefly when a draught hits
    // it. So every LED reads its own position from a 2D noise field that moves over
    // time (smooth flicker), and a rare random gust darkens all LEDs at once and then
    // fades out. Bright moments shift towards yellow, dim ones towards deep orange.
    void StripController::renderCandle(uint32_t now)
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
            // Plain RGB instead of CHSV: CHSV caps the summed channels, full red + green is brighter.
            const uint8_t green = lerp8by8(FLAME_GREEN_MIN, FLAME_GREEN_MAX, noise);
            _strip[index]       = CRGB(255, green, 0).nscale8_video(value);
        }
    }

    // ===========================================================================
    // =============================   PERSISTENCE   =============================
    // ===========================================================================
    //
    //   load   Reads the settings from NVS, the member defaults apply on the first boot.
    //   save   Writes the settings to NVS.
    //
    //   Keys "effect" and "colorpos": an earlier firmware stored "mode", "hue", "sat"
    //   and "bri" with other meanings, those are left alone and ignored.
    //

    void StripController::load()
    {
        // Read-write, so the namespace exists on the first boot and the read-only error log is skipped
        _prefs.begin(PREFS_NAMESPACE, false);
        _isOn          = _prefs.getBool("on", _isOn);
        _colorPosition = _prefs.getUChar("colorpos", _colorPosition);
        // A value from a firmware with more effects falls back to the default
        const uint8_t storedMode = _prefs.getUChar("effect", static_cast<uint8_t>(_mode));
        if (storedMode < STRIP_MODE_COUNT)
        {
            _mode = static_cast<StripMode>(storedMode);
        }
        _prefs.end();
    }

    void StripController::save()
    {
        _prefs.begin(PREFS_NAMESPACE, false);
        _prefs.putBool("on", _isOn);
        _prefs.putUChar("effect", static_cast<uint8_t>(_mode));
        _prefs.putUChar("colorpos", _colorPosition);
        _prefs.end();
        Serial.println("[StripController] Settings saved to NVS");
    }
}
