#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Preferences.h>

#include <MiniServer.h>
#include <routes/routes.sensor.h>

#include <Adafruit_BME280.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <display/historyGraph.h>
#include <display/theme.h>

Adafruit_BME280 bme;
TFT_eSPI display = TFT_eSPI();
Preferences prefs;

// XPT2046 touch chip on this board sits on its own SPI pins, separate from the
// TFT bus (which TFT_eSPI drives directly, bypassing the Arduino SPI object).
// So the global SPI object is free to remap to the touch pins here.
XPT2046_Touchscreen touchScreen(CYD_TOUCH_CS, CYD_TOUCH_IRQ);

// This library returns raw ADC readings (0-4095), not screen pixels, so the
// raw range touched at two known screen points must be captured once and
// stored to linearly map future touches to pixel coordinates.
struct TouchCalibration
{
  uint16_t rawX1, rawY1; // captured at (calMargin, calMargin)
  uint16_t rawX2, rawY2; // captured at (width - calMargin, height - calMargin)
};
TouchCalibration touchCal;
const int calMargin = 20;

EspWeb::MiniServer Server;

enum class Metric
{
  TEMPERATURE,
  HUMIDITY,
  PRESSURE
};

HistoryGraph tempGraph("Temperatur", "C");
HistoryGraph humGraph("Feuchte", "%");
HistoryGraph presGraph("Druck", "hPa");
HistoryGraph *graphs[] = {&tempGraph, &humGraph, &presGraph};

bool showingDetail  = false;
Metric detailMetric = Metric::TEMPERATURE;

// The ILI9341 backlight LED draws far more current than the ESP32 + sensors
// combined, so it gets dimmed and eventually switched off entirely after a
// period without touch input instead of staying lit continuously.
const int backlightFullDuty           = 255;
const int backlightDimDuty            = 20;
const unsigned long idleDimTimeoutMs  = 15000;  // dim after 15s idle
const unsigned long idleOffTimeoutMs  = 120000; // off after 2min idle

enum class BacklightState
{
  FULL,
  DIM,
  OFF
};
BacklightState backlightState = BacklightState::FULL;
unsigned long lastTouchMs     = 0;

void setBacklight(BacklightState state)
{
  if (state == backlightState)
    return;

  switch (state)
  {
  case BacklightState::FULL:
    analogWrite(TFT_BL, backlightFullDuty);
    break;
  case BacklightState::DIM:
    analogWrite(TFT_BL, backlightDimDuty);
    break;
  case BacklightState::OFF:
    analogWrite(TFT_BL, 0);
    break;
  }
  backlightState = state;
}

TS_Point waitForTap()
{
  while (!touchScreen.touched())
    delay(10);
  TS_Point p = touchScreen.getPoint();
  delay(300); // debounce
  while (touchScreen.touched())
    delay(10);
  return p;
}

void runTouchCalibration()
{
  const int w = display.width();
  const int h = display.height();

  display.fillScreen(theme::background);
  display.setTextColor(theme::text, theme::background);
  display.setTextDatum(MC_DATUM);
  display.setTextSize(2);
  display.drawString("Oben links beruehren", w / 2, h / 2);
  display.fillCircle(calMargin, calMargin, 5, theme::accent);
  TS_Point p1 = waitForTap();

  display.fillScreen(theme::background);
  display.drawString("Unten rechts beruehren", w / 2, h / 2);
  display.fillCircle(w - calMargin, h - calMargin, 5, theme::accent);
  TS_Point p2 = waitForTap();

  touchCal.rawX1 = p1.x;
  touchCal.rawY1 = p1.y;
  touchCal.rawX2 = p2.x;
  touchCal.rawY2 = p2.y;

  prefs.begin("touch", false);
  prefs.putBytes("cal", &touchCal, sizeof(touchCal));
  prefs.end();
}

void setupTouchCalibration()
{
  prefs.begin("touch", true);
  bool haveCal = prefs.getBytesLength("cal") == sizeof(touchCal);
  if (haveCal)
    prefs.getBytes("cal", &touchCal, sizeof(touchCal));
  prefs.end();

  if (!haveCal)
    runTouchCalibration();
}

bool getScreenTouch(uint16_t &sx, uint16_t &sy)
{
  if (!touchScreen.touched())
    return false;

  TS_Point p = touchScreen.getPoint();
  long x     = map(p.x, touchCal.rawX1, touchCal.rawX2, calMargin, display.width() - calMargin);
  long y     = map(p.y, touchCal.rawY1, touchCal.rawY2, calMargin, display.height() - calMargin);
  sx         = constrain(x, 0, display.width() - 1);
  sy         = constrain(y, 0, display.height() - 1);
  return true;
}

Metric metricAtY(int y)
{
  const int rowH = display.height() / 3;
  if (y < rowH)
    return Metric::TEMPERATURE;
  if (y < rowH * 2)
    return Metric::HUMIDITY;
  return Metric::PRESSURE;
}

void drawOverview(float temp, float hum, float pres)
{
  const int w    = display.width();
  const int h    = display.height();
  const int rowH = h / 3;

  display.fillScreen(theme::background);
  display.setTextDatum(MC_DATUM);

  display.setTextColor(theme::accent, theme::background);
  display.setTextSize(2);
  display.drawString("Temperatur", w / 2, rowH * 0 + 16);
  display.setTextColor(theme::text, theme::background);
  display.setTextSize(4);
  display.drawString(String(temp, 1) + " C", w / 2, rowH * 0 + rowH / 2 + 10);

  display.setTextColor(theme::accent, theme::background);
  display.setTextSize(2);
  display.drawString("Feuchte", w / 2, rowH * 1 + 16);
  display.setTextColor(theme::text, theme::background);
  display.setTextSize(3);
  display.drawString(String(hum, 1) + " %", w / 2, rowH * 1 + rowH / 2 + 10);

  display.setTextColor(theme::accent, theme::background);
  display.setTextSize(2);
  display.drawString("Druck", w / 2, rowH * 2 + 16);
  display.setTextColor(theme::text, theme::background);
  display.setTextSize(3);
  display.drawString(String(pres, 1) + " hPa", w / 2, rowH * 2 + rowH / 2 + 10);

  display.drawFastHLine(0, rowH, w, theme::line);
  display.drawFastHLine(0, rowH * 2, w, theme::line);
}

void setup()
{
  Serial.begin(115200);

  Wire.begin(CYD_I2C_SDA, CYD_I2C_SCL);

  bme.begin(0x76);

  display.init();
  display.setRotation(1);
  display.fillScreen(theme::background);
  display.setTextColor(theme::text, theme::background);
  analogWrite(TFT_BL, backlightFullDuty);
  lastTouchMs = millis();

  SPI.begin(CYD_TOUCH_CLK, CYD_TOUCH_DO, CYD_TOUCH_DIN, CYD_TOUCH_CS);
  touchScreen.begin();
  touchScreen.setRotation(1);
  setupTouchCalibration();

  Server.registerRouter(routes_sensor::Router());

  setCpuFrequencyMhz(80);
  Server.start(80);
  WiFi.setSleep(true);
}

void loop()
{
  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  tempGraph.push(temp);
  humGraph.push(hum);
  presGraph.push(pres);

  // Skip the (SPI-heavy) redraw entirely while the backlight is off - nothing
  // is visible anyway, only the history buffers above need to keep filling.
  if (backlightState != BacklightState::OFF)
  {
    if (showingDetail)
      graphs[(int)detailMetric]->draw(display);
    else
      drawOverview(temp, hum, pres);
  }

  const int sampleIntervalMs = 10000; // sensor values change slowly, no need to redraw every 2s
  const int pollStepMs       = 50;
  for (int waited = 0; waited < sampleIntervalMs; waited += pollStepMs)
  {
    uint16_t tx, ty;
    if (getScreenTouch(tx, ty))
    {
      // If the screen was off, this tap only wakes it back up and shows the
      // current view again - it doesn't also act as a row tap.
      bool wasOff = backlightState == BacklightState::OFF;
      lastTouchMs = millis();
      setBacklight(BacklightState::FULL);

      if (!wasOff)
      {
        if (showingDetail)
          showingDetail = false;
        else
        {
          detailMetric  = metricAtY(ty);
          showingDetail = true;
        }
      }

      if (showingDetail)
        graphs[(int)detailMetric]->draw(display);
      else
        drawOverview(temp, hum, pres);

      delay(300); // debounce
    }
    else
    {
      unsigned long idleMs = millis() - lastTouchMs;
      if (idleMs > idleOffTimeoutMs)
        setBacklight(BacklightState::OFF);
      else if (idleMs > idleDimTimeoutMs)
        setBacklight(BacklightState::DIM);
    }
    delay(pollStepMs);
  }
}
