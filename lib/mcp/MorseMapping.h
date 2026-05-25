#pragma once

#include <Arduino.h>
#include <string>
#include <vector>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace morseCode
{
    class Morse
    {
    private:
        int _timeUnit = 500;
        gpio_num_t _pin;

        static std::string getMorseCode(char letter)
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

        static std::vector<int> codeToDelays(char letter, int timeUnit)
        {
            std::string code = getMorseCode(letter);
            std::vector<int> delays;
            for (char c : code)
                delays.push_back(c == '.' ? timeUnit : timeUnit * 3);
            return delays;
        }

        void _morseLetter(char letter)
        {
            std::vector<int> delays = codeToDelays(letter, _timeUnit);
            Serial.printf("%c | %s\n", letter, getMorseCode(letter).c_str());
            for (int delay : delays)
            {
                Serial.printf("  | Blinking %d ms\n", delay);
                gpio_set_level(_pin, 1);
                vTaskDelay(delay / portTICK_PERIOD_MS);
                gpio_set_level(_pin, 0);
                vTaskDelay(_timeUnit / portTICK_PERIOD_MS);
            }
        }

    public:
        Morse(gpio_num_t pin) : _pin(pin)
        {
            gpio_reset_pin(_pin);
            gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
        }

        void morse(const std::string &message)
        {
            const int wordGap = 7 * _timeUnit;
            for (const char c : message)
            {
                if (c == ' ')
                    vTaskDelay(wordGap / portTICK_PERIOD_MS);
                else
                    _morseLetter(c);
            }
        }
    };
}
