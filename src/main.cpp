#include <Arduino.h>

#include <MiniServer.h>

#include <routes/routes.gpt.h>
#include <DS18B20Client.h>

DS18B20Client tempTest;

EspWeb::MiniServer Server;

void setup()
{
  Serial.begin(115200);
  tempTest.begin();

  Server.registerRouter(routes_gpt::Router());

  Server.index("/web/index.html");
  Server.staticFile("/admin/settings", "/web/settings.html");
  Server.setCustomLink("GPT Settings", "/admin/settings");

  Server.start(80);
}

void loop()
{
  String result = tempTest.readTemperature(JsonDocument());
  Serial.println("[temp] " + result);
  delay(2000);
}
