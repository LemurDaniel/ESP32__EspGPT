#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <toolTypes.h>

class WebClient : public Mcp
{
private:
    WiFiClientSecure _client;

public:
    WebClient()
    {
        Tool t("call_url", "Call a URL via HTTP GET and return the response body");
        t.withSchema(Schema{}.string("Url", "The URL to call").required("Url"));
        t.onCall(std::bind(&WebClient::get, this, std::placeholders::_1));
        registerTool(t);
    }

    WebClient &begin()
    {
        _client.setInsecure();
        return *this;
    }

    String get(JsonDocument doc)
    {
        String url = doc["Url"].as<String>();
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
