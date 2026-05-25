#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "toolTypes.h"

class WebClient : public Mcp
{
private:
    WiFiClientSecure _client;

public:
    WebClient()
    {
        registerTool("call_url",
                     "Call a URL via HTTP GET and return the response body",
                     R"({"type":"object","properties":{"Url":{"type":"string","description":"The URL to call"}},"required":["Url"]})",
                     [this](JsonDocument doc) -> String
                     { return get(doc["Url"].as<String>()); });
    }

    WebClient &begin()
    {
        _client.setInsecure();
        return *this;
    }

    String get(const String &url)
    {
        Serial.printf("[web] GET %s\n", url.c_str());

        HTTPClient http;
        if (!http.begin(_client, url))
        {
            Serial.println("[web] http.begin() failed");
            return String("Error: http.begin() failed");
        }

        int status = http.GET();
        Serial.printf("[web] HTTP status: %d\n", status);

        String response = http.getString();
        http.end();
        return response;
    }
};
