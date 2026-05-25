#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>
#include <toolTypes.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class LEDClient : public Mcp
{
private:
    gpio_num_t _pin;
    int _state = 0;
    int _morseTimeUnit = 500;

    static std::string _getMorseCode(char letter)
    {
        switch (letter)
        {
        case 'A': case 'a': return ".-";
        case 'B': case 'b': return "-...";
        case 'C': case 'c': return "-.-.";
        case 'D': case 'd': return "-..";
        case 'E': case 'e': return ".";
        case 'F': case 'f': return "..-.";
        case 'G': case 'g': return "--.";
        case 'H': case 'h': return "....";
        case 'I': case 'i': return "..";
        case 'J': case 'j': return ".---";
        case 'K': case 'k': return "-.-";
        case 'L': case 'l': return ".-..";
        case 'M': case 'm': return "--";
        case 'N': case 'n': return "-.";
        case 'O': case 'o': return "---";
        case 'P': case 'p': return ".--.";
        case 'Q': case 'q': return "--.-";
        case 'R': case 'r': return ".-.";
        case 'S': case 's': return "...";
        case 'T': case 't': return "-";
        case 'U': case 'u': return "..-";
        case 'V': case 'v': return "...-";
        case 'W': case 'w': return ".--";
        case 'X': case 'x': return "-..-";
        case 'Y': case 'y': return "-.--";
        case 'Z': case 'z': return "--..";
        default: return "";
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
        registerTool("led_on",
                     "Turn the LED on",
                     R"({"type":"object","properties":{}})",
                     [this](JsonDocument doc) -> String
                     { on(); return "LED turned on"; });

        registerTool("led_off",
                     "Turn the LED off",
                     R"({"type":"object","properties":{}})",
                     [this](JsonDocument doc) -> String
                     { off(); return "LED turned off"; });

        registerTool("led_is_on",
                     "Check whether the LED is currently on",
                     R"({"type":"object","properties":{}})",
                     [this](JsonDocument doc) -> String
                     { return isOn() ? "LED is on" : "LED is off"; });

        registerTool("led_morse",
                     "Blink a message in morse code on the LED",
                     R"({"type":"object","properties":{"message":{"type":"string","description":"Text to morse"}}})",
                     [this](JsonDocument doc) -> String
                     {
                         std::string msg = doc["arguments"]["message"].as<std::string>();
                         morse(msg);
                         return "Morse sent: " + String(msg.c_str());
                     });
    }

    LEDClient &begin(gpio_num_t pin)
    {
        _pin = pin;
        gpio_reset_pin(_pin);
        gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
        return *this;
    }

    bool isOn() { return _state; }

    void on()
    {
        _state = 1;
        gpio_set_level(_pin, 1);
    }

    void off()
    {
        _state = 0;
        gpio_set_level(_pin, 0);
    }

    void morse(const std::string &message)
    {
        const int wordGap = 7 * _morseTimeUnit;
        for (const char c : message)
        {
            if (c == ' ')
                vTaskDelay(wordGap / portTICK_PERIOD_MS);
            else
                _morseLetter(c);
        }
    }
};
