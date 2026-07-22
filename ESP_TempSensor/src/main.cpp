#include <Arduino.h>
#include <WiFi.h>

#include <MiniServer.h>
#include <routes/routes.sensor.h>

#include <Adafruit_BME280.h>
#include <TFT_eSPI.h>

Adafruit_BME280 bme;
TFT_eSPI display = TFT_eSPI();

EspWeb::MiniServer Server;

void setup()
{
  Serial.begin(115200);

  Wire.begin(CYD_I2C_SDA, CYD_I2C_SCL);

  bme.begin(0x76);

  display.init();
  display.setRotation(1);
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_WHITE, TFT_BLACK);

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

  display.fillScreen(TFT_BLACK);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temp:    ");
  display.setTextSize(3);
  display.print(temp, 1);
  display.println(" C");

  display.setTextSize(1);
  display.setCursor(0, 70);
  display.print("Feuchte: ");
  display.print(hum, 1);
  display.println(" %");

  display.setCursor(0, 90);
  display.print("Druck:   ");
  display.print(pres, 1);
  display.println(" hPa");

  delay(2000);
}
