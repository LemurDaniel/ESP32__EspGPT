#pragma once

#include <Arduino.h>
#include <tool.h>

#include <Adafruit_BME280.h>

class BME280Client : public Mcp
{
private:
    Adafruit_BME280 _bme;

public:
    BME280Client()
    {
        Tool tReadHum("read_humidity", "Read the current humidity from the BME280 sensor in Percent");
        tReadHum.does(std::bind(&BME280Client::readHumidity, this, std::placeholders::_1));
        registerTool(tReadHum);

        Tool tReadTemp("read_temperature", "Read the current temperature from the BME280 sensor in Celsius");
        tReadTemp.does(std::bind(&BME280Client::readTemperature, this, std::placeholders::_1));
        registerTool(tReadTemp);

        Tool tGetPressure("read_pressure", "Read the current pressure from the BME280 sensor in hecto pascall");
        tGetPressure.does(std::bind(&BME280Client::readPressure, this, std::placeholders::_1));
        registerTool(tGetPressure);
    }

    BME280Client &begin()
    {
        if (!_bme.begin(0x76))
        {
            Serial.println("Kein BME280 gefunden! Adresse oder Verkabelung prüfen.");
        }
        return *this;
    }

    String readTemperature(JsonDocument)
    {
        return "Temperature is " + String(_bme.readTemperature()) + "°C";
    }
    String readPressure(JsonDocument)
    {
        return "Pressure is" + String(_bme.readPressure() / 100.0F) + " hPa";
    }
    String readHumidity(JsonDocument)
    {
        return "Humidity is " + String(_bme.readHumidity());
    }
};
