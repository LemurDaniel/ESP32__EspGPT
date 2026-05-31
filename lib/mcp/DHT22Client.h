#pragma once

#include <Arduino.h>
#include <DHT.h>
#include <tool.h>

#define DHT22_DEFAULT_GPIO 15

class DHT22Client : public Mcp
{
private:
    uint8_t _pin = DHT22_DEFAULT_GPIO;
    DHT _dht;

public:
    DHT22Client() : _dht(DHT22_DEFAULT_GPIO, DHT22)
    {
        Tool tReadTemp("read_temperature", "Read the current temperature from the DHT22 sensor in Celsius");
        tReadTemp.does(std::bind(&DHT22Client::readTemperature, this, std::placeholders::_1));
        registerTool(tReadTemp);

        Tool tReadHum("read_humidity", "Read the current humidity from the DHT22 sensor in Percent");
        tReadHum.does(std::bind(&DHT22Client::readHumidity, this, std::placeholders::_1));
        registerTool(tReadHum);

        Tool tSetPin("dht22_set_pin", "Set the GPIO pin for the DHT22 sensor");
        tSetPin.param("pin").description("GPIO pin number").integer();
        tSetPin.does(std::bind(&DHT22Client::setPin, this, std::placeholders::_1));
        registerTool(tSetPin);

        Tool tGetPin("dht22_get_pin", "Get the GPIO pin currently used by the DHT22 sensor");
        tGetPin.does(std::bind(&DHT22Client::getPin, this, std::placeholders::_1));
        registerTool(tGetPin);
    }

    DHT22Client &begin()
    {
        _dht.begin();
        return *this;
    }

    String getPin(JsonDocument)
    {
        return "DHT22 is read from GPIO " + String(_pin);
    }

    String setPin(JsonDocument doc)
    {
        _pin = doc["pin"].as<uint8_t>();
        _dht = DHT(_pin, DHT22);
        _dht.begin();
        return "DHT22 sensor moved to GPIO " + String(_pin);
    }

    String readTemperature(JsonDocument)
    {
        float temp = _dht.readTemperature();
        if (isnan(temp))
            return "Error: DHT22 sensor not found or read failed";
        return "Temperature is " + String(temp) + " °C";
    }

    String readHumidity(JsonDocument)
    {
        float hum = _dht.readHumidity();
        if (isnan(hum))
            return "Error: DHT22 sensor not found or read failed";
        return "Humidity is " + String(hum) + " %";
    }
};
