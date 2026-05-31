#include <Arduino.h>

#include <MiniServer.h>

#include <routes/routes.gpt.h>
#include <Adafruit_BME280.h>

EspWeb::MiniServer Server;
Adafruit_BME280 bme;

void setup()
{
  Serial.begin(115200);

  if (!bme.begin(0x76))
    Serial.println("BME280: Kein Sensor gefunden! Adresse oder Verkabelung prüfen.");

  Server.registerRouter(routes_gpt::Router());

  Server.index("/web/index.html");
  Server.staticFile("/admin/settings", "/web/settings.html");
  Server.setCustomLink("GPT Settings", "/admin/settings");

  Server.start(80);
}

void loop()
{
  Serial.printf("Temp: %.1f °C  |  Druck: %.1f hPa  |  Feuchte: %.1f %%\n",
                bme.readTemperature(),
                bme.readPressure() / 100.0F,
                bme.readHumidity());
  delay(2000);
}
