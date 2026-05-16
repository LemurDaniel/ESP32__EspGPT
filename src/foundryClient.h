#pragma once

#include <Arduino.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class AzureFoundryClient
{

private:
    int _status = 0;
    String _error;

public:
    int status() const { return _status; }
    String error() const { return _error; }

private:
    WiFiClientSecure _client;

    String _baseUrl;
    String _apiKey;
    String _systemPrompt;

    String buildJson(const String &prompt, const String &model)
    {
        JsonDocument reqDoc;
        reqDoc["model"] = model;
        JsonArray messages = reqDoc["messages"].to<JsonArray>();

        JsonObject sys = messages.add<JsonObject>();
        sys["role"] = "system";
        sys["content"] = _systemPrompt;

        JsonObject usr = messages.add<JsonObject>();
        usr["role"] = "user";
        usr["content"] = prompt;

        String body;
        serializeJson(reqDoc, body);
    }

    String post(const String url, const String body)
    {
        HTTPClient http;

        if (!http.begin(_client, url))
        {
            _error = "http.begin() fehlgeschlagen";
            return "";
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("api-key", _apiKey);

        _status = http.POST(body);
        if (_status <= 0)
        {
            _error = HTTPClient::errorToString(_status);
            http.end();
            return "";
        }

        String response = http.getString();
        http.end();

        if (_status != 200)
        {
            // Azure liefert bei Fehlern JSON mit Details (z.B. falscher Key/Deployment)
            _error = response;
            return "";
        }

        // Filter = nur das Noetige aus dem JSON ziehen, spart auf dem ESP wertvollen RAM.
        JsonDocument temp;
        JsonDocument filter;
        filter["choices"][0]["message"]["content"] = true;

        DeserializationError err = deserializeJson(temp, response, DeserializationOption::Filter(filter));
        if (err)
        {
            _error = String("JSON-Parsefehler: ") + err.c_str();
            return "";
        }

        return temp["choices"][0]["message"]["content"];
    }

public:
    // baseUrl ohne abschliessenden Slash, z.B.
    // "https://testingaihvj.cognitiveservices.azure.com"
    void begin(String url, String key)
    {
        _baseUrl = url;
        _apiKey = key;

        // NUR fuer Tests: keine Zertifikatspruefung.
        // Produktion: _client.setCACert(rootCaPem);  statt setInsecure()
        _client.setInsecure();
    }

    String systemPrompt(const String &prompt)
    {
        _systemPrompt = prompt;
    }

    /*
       Schickt System- + User-Prompt, liefert die Antwort des Modells zurueck.
       Bei einem Fehler: leerer String. Details dann ueber
       lastStatus() (HTTP-Code) und lastError() (Fehlertext / Azure-JSON).
   */
    String ask(const String &prompt, const String &model)
    {
        _error = "";
        _status = 0;

        const String body = buildJson(prompt, model);
        String url = _baseUrl + "/openai/v1/chat/completions";

        return post(url, body);
    }
};