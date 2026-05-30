#include <Arduino.h>

#include <MiniServer.h>

#include <routes/routes.gpt.h>
#include <DS18B20Client.h>

EspWeb::MiniServer Server;

void setup()
{
  Serial.begin(115200);

  Server.registerRouter(routes_gpt::Router());

  Server.index("/web/index.html");
  Server.staticFile("/admin/settings", "/web/settings.html");
  Server.setCustomLink("GPT Settings", "/admin/settings");

  Server.start(80);
}


void loop()
{
  vTaskDelete(NULL); // Loop-Task komplett entfernen, FreeRTOS gibt Ressourcen frei
}
