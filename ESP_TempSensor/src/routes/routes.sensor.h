#pragma once

#include <router/router.h>
#include <Adafruit_BME280.h>

namespace routes_sensor
{

    class Router : public EspWeb::Router
    {
    public:
        Router();

    private:
        static Adafruit_BME280 _bme;
        static void get_sensor(EspWeb::Request &request, EspWeb::Response &response);
    };

}
