#include <Arduino.h>
#include <WiFi.h>

#include <MiniServer.h>
#include <routes/routes.sensor.h>

#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR    0x3C

Adafruit_BME280  bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

EspWeb::MiniServer Server;

void setup()
{
  Serial.begin(115200);

  bme.begin(0x76);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    Serial.println("SSD1306: Kein Display gefunden!");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();

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

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temp:    ");
  display.setTextSize(2);
  display.print(temp, 1);
  display.println(" C");

  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print("Feuchte: ");
  display.print(hum, 1);
  display.println(" %");

  display.setCursor(0, 50);
  display.print("Druck:   ");
  display.print(pres, 1);
  display.println(" hPa");

  display.display();

  delay(2000);
}
