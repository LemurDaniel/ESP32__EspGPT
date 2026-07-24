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

  if (showingDetail)
    graphs[(int)detailMetric]->draw(display);
  else
    drawOverview(temp, hum, pres);

  const int sampleIntervalMs = 2000;
  const int pollStepMs       = 50;
  for (int waited = 0; waited < sampleIntervalMs; waited += pollStepMs)
  {
    uint16_t tx, ty;
    if (getScreenTouch(tx, ty))
    {
      if (showingDetail)
      {
        showingDetail = false;
        drawOverview(temp, hum, pres);
      }
      else
      {
        detailMetric  = metricAtY(ty);
        showingDetail = true;
        graphs[(int)detailMetric]->draw(display);
      }
      delay(300); // debounce
    }
    delay(pollStepMs);
  }
}
