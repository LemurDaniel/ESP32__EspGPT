#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>
#include <tool.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class LEDClient : public Mcp
{
private:
    static std::string _getMorseCode(char letter)
    {
        switch (letter)
        {
        case 'A':
        case 'a':
            return ".-";
        case 'B':
        case 'b':
            return "-...";
        case 'C':
        case 'c':
            return "-.-.";
        case 'D':
        case 'd':
            return "-..";
        case 'E':
        case 'e':
            return ".";
        case 'F':
        case 'f':
            return "..-.";
        case 'G':
        case 'g':
            return "--.";
        case 'H':
        case 'h':
            return "....";
        case 'I':
        case 'i':
            return "..";
        case 'J':
        case 'j':
            return ".---";
        case 'K':
        case 'k':
            return "-.-";
        case 'L':
        case 'l':
            return ".-..";
        case 'M':
        case 'm':
            return "--";
        case 'N':
        case 'n':
            return "-.";
        case 'O':
        case 'o':
            return "---";
        case 'P':
        case 'p':
            return ".--.";
        case 'Q':
        case 'q':
            return "--.-";
        case 'R':
        case 'r':
            return ".-.";
        case 'S':
        case 's':
            return "...";
        case 'T':
        case 't':
            return "-";
        case 'U':
        case 'u':
            return "..-";
        case 'V':
        case 'v':
            return "...-";
        case 'W':
        case 'w':
            return ".--";
        case 'X':
        case 'x':
            return "-..-";
        case 'Y':
        case 'y':
            return "-.--";
        case 'Z':
        case 'z':
            return "--..";
        default:
            return "";
        }
    }

    void _morseLetter(char letter)
    {
        std::string code = _getMorseCode(letter);
        Serial.printf("%c | %s\n", letter, code.c_str());
        for (char c : code)
        {
            int duration = (c == '.') ? _morseTimeUnit : _morseTimeUnit * 3;
            Serial.printf("  | Blinking %d ms\n", duration);
            gpio_set_level(_pin, 1);
            vTaskDelay(duration / portTICK_PERIOD_MS);
            gpio_set_level(_pin, 0);
            vTaskDelay(_morseTimeUnit / portTICK_PERIOD_MS);
        }
    }

public:
    LEDClient()
    {

        Tool ledOn("led_on", "Turn the LED on");
        ledOn.does(std::bind(&LEDClient::on, this, std::placeholders::_1));
        registerTool(ledOn);

        Tool ledOff("led_off", "Turn the LED off");
        ledOff.does(std::bind(&LEDClient::off, this, std::placeholders::_1));
        registerTool(ledOff);

        Tool ledIsOn("led_is_on", "Check whether the LED is currently on");
        ledIsOn.does(std::bind(&LEDClient::isOn, this, std::placeholders::_1));
        registerTool(ledIsOn);

        Tool ledMorse("led_morse", "Blink a message in morse code on the LED");
        ledMorse.param("message").description("Text to morse").string();
        ledMorse.does(std::bind(&LEDClient::morse, this, std::placeholders::_1));
        registerTool(ledMorse);

        Tool ledSet("led_set", "Set the GPIO pin for the LED");
        ledSet.param("pin").description("GPIO pin number (gpio_num_t)").integer();
        ledSet.does(std::bind(&LEDClient::set, this, std::placeholders::_1));
        registerTool(ledSet);

        Tool ledGpio("led_gpio", "Get the current GPIO pin of the LED");
        ledGpio.does(std::bind(&LEDClient::gpio, this, std::placeholders::_1));
        registerTool(ledGpio);
    }

private:
    gpio_num_t _pin = GPIO_NUM_2;
    int _state = 0;
    int _morseTimeUnit = 500;

public:
    String gpio(JsonDocument)
    {
        return "Using PIN " + String(_pin);
    }

    String set(JsonDocument doc)
    {
        _pin = doc["pin"].as<gpio_num_t>();
        gpio_reset_pin(_pin);
        gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
        return "Using PIN " + String(_pin);
    }

    String on(JsonDocument)
    {
        _state = 1;
        gpio_set_level(_pin, 1);
        return "LED turned on";
    }

    String off(JsonDocument)
    {
        _state = 0;
        gpio_set_level(_pin, 0);
        return "LED turned off";
    }

    String isOn(JsonDocument)
    {
        return _state ? "LED is on" : "LED is off";
    }

    String morse(JsonDocument doc)
    {
        std::string msg = doc["arguments"]["message"].as<std::string>();

        const int wordGap = 7 * _morseTimeUnit;
        for (const char c : msg)
        {
            if (c == ' ')
                vTaskDelay(wordGap / portTICK_PERIOD_MS);
            else
                _morseLetter(c);
        }
        return "";
    }
};
