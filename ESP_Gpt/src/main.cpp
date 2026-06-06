#include <Arduino.h>

#include <MiniServer.h>

#include <routes/routes.gpt.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

EspWeb::MiniServer Server;
Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 nicht gefunden!");
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Hallo!");
    display.display();
    Serial.println("Display OK");
  }

  Server.registerRouter(routes_gpt::Router());

  Server.index("/web/index.html");
  Server.staticFile("/admin/settings", "/web/settings.html");
  Server.setCustomLink("GPT Settings", "/admin/settings");

  setCpuFrequencyMhz(80);
  Server.start(80);
  WiFi.setSleep(true);
}

void loop()
{
  vTaskDelete(NULL);
}
