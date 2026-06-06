#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <tool.h>

#define TEMPERATURE_DEFAULT_GPIO 4

class DS18B20Client : public Mcp
{
private:
    uint8_t _pin = TEMPERATURE_DEFAULT_GPIO;

    OneWire _oneWire;
    DallasTemperature _sensor;

public:
    DS18B20Client() : _oneWire(TEMPERATURE_DEFAULT_GPIO), _sensor(&_oneWire)
    {
        Tool tRead("read_temperature", "Read the current temperature from the DS18B20 sensor in Celsius");
        tRead.does(std::bind(&DS18B20Client::readTemperature, this, std::placeholders::_1));
        registerTool(tRead);

        Tool tSetPin("temperature_set_pin", "Set the GPIO pin for the DS18B20 temperature sensor");
        tSetPin.param("pin").description("GPIO pin number").integer();
        tSetPin.does(std::bind(&DS18B20Client::setPin, this, std::placeholders::_1));
        registerTool(tSetPin);

        Tool tGetPin("temperature_get_pin", "Get the GPIO pin for the DS18B20 temperature sensor");
        tGetPin.param("pin").description("GPIO pin number").integer();
        tGetPin.does(std::bind(&DS18B20Client::getPin, this, std::placeholders::_1));
        registerTool(tGetPin);
    }

    DS18B20Client &begin()
    {
        _oneWire.begin(_pin);
        _sensor.begin();
        return *this;
    }

    String getPin(JsonDocument)
    {
        return "Temperature is read from GPIO " + String(_pin);
    }

    String setPin(JsonDocument doc)
    {
        _pin = doc["pin"].as<uint8_t>();
        _oneWire.begin(_pin);
        _sensor.begin();
        return "Temperature sensor moved to GPIO " + String(_pin);
    }

    String readTemperature(JsonDocument)
    {
        _sensor.requestTemperatures();
        float temp = _sensor.getTempCByIndex(0);
        if (temp == DEVICE_DISCONNECTED_C)
            return "Error: sensor not found or disconnected";

        return String(temp) + " °C";
    }
};
