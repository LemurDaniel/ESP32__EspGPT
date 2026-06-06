#include <routes/routes.sensor.h>

namespace routes_sensor
{
    Adafruit_BME280 Router::_bme;

    Router::Router()
    {
        if (!_bme.begin(0x76))
            Serial.println("BME280: Kein Sensor gefunden! Adresse oder Verkabelung prüfen.");

        route("GET", "/sensor", get_sensor);
    }

    void Router::get_sensor(EspWeb::Request &request, EspWeb::Response &response)
    {
        JsonDocument doc;
        doc["temperature"] = _bme.readTemperature();
        doc["humidity"]    = _bme.readHumidity();
        doc["pressure"]    = _bme.readPressure() / 100.0F;
        response.json(doc).OK();
    }
}
