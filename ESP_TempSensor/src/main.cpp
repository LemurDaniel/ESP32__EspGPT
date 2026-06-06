#include <Arduino.h>
#include <WiFi.h>

#include <MiniServer.h>
#include <routes/routes.sensor.h>

EspWeb::MiniServer Server;

void setup()
{
  Serial.begin(115200);

  Server.registerRouter(routes_sensor::Router());

  setCpuFrequencyMhz(80);
  Server.start(80);
  WiFi.setSleep(true);
}

void loop()
{
  vTaskDelete(NULL); // Loop-Task komplett entfernen, FreeRTOS gibt Ressourcen frei
}