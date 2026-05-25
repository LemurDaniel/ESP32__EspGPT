#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <toolTypes.h>

class AzureFoundryClient
{
private:
    int _status = 0;
    String _error;

    WiFiClientSecure _client;
    String _baseUrl;
    String _apiKey;
    String _model;
    String _systemPrompt;

    std::map<String, Tool> _tools;

    JsonDocument _post(JsonDocument &req)
    {
        if (!_systemPrompt.isEmpty())
            req["instructions"] = _systemPrompt;

        if (!_tools.empty())
        {
            JsonArray toolsArray = req["tools"].to<JsonArray>();
            for (const auto &entry : _tools)
            {
                JsonObject tool = toolsArray.add<JsonObject>();
                tool["type"] = "function";
                tool["name"] = entry.first;
                tool["description"] = entry.second.description;

                JsonDocument paramsDoc;
                deserializeJson(paramsDoc, entry.second.paramsSchema);
                tool["parameters"] = paramsDoc;
            }
        }

        HTTPClient http;
        if (!http.begin(_client, _baseUrl + "/openai/v1/responses"))
        {
            _error = "http.begin() failed";
            _status = -1;
            return JsonDocument();
        }
        http.addHeader("Content-Type", "application/json");
        http.addHeader("api-key", _apiKey);

        String body;
        serializeJson(req, body);
        _status = http.POST(body);

        String response = http.getString();
        http.end();

        if (_status != 200)
        {
            _error = response;
            return JsonDocument();
        }

        if (response.isEmpty())
            return JsonDocument();

        JsonDocument doc;
        if (deserializeJson(doc, response))
        {
            _error = "JSON parse error";
            return JsonDocument();
        }

        return doc;
    }

public:
    int status() const { return _status; }
    String error() const { return _error; }

    // baseUrl ohne abschliessenden Slash, z.B.
    // "https://testingaihvj.cognitiveservices.azure.com"
    void begin(const String &url, const String &key, const String &model, const String &systemPrompt = "")
    {
        _baseUrl = url;
        _apiKey = key;
        _model = model;
        _systemPrompt = systemPrompt;
        // NUR fuer Tests: keine Zertifikatspruefung.
        // Produktion: _client.setCACert(rootCaPem); statt setInsecure()
        _client.setInsecure();
    }

    void registerTool(const String &name, const String &description, const String &paramsSchema, ToolHandler handler)
    {
        _tools.insert({name, {name, description, paramsSchema, handler}});
    }

    JsonDocument complete(const String &userMessage, const String &previousResponseId = "")
    {
        Serial.println("[client] Last Response Id: " + previousResponseId);

        JsonDocument req;
        req["model"] = _model;
        if (!previousResponseId.isEmpty())
            req["previous_response_id"] = previousResponseId;

        JsonArray input = req["input"].to<JsonArray>();
        JsonObject msg = input.add<JsonObject>();
        msg["type"] = "message";
        msg["role"] = "user";
        msg["content"] = userMessage;

        return _post(req);
    }

    JsonDocument complete(const JsonDocument &toolResults, const String &previousResponseId)
    {
        Serial.println("[client] Last Response Id: " + previousResponseId);

        JsonDocument req;
        req["model"] = _model;
        req["previous_response_id"] = previousResponseId;
        req["input"] = toolResults;

        return _post(req);
    }
};
