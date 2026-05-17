#pragma once

#include <Arduino.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

class AzureFoundryClient
{

    /*-------------------------------------------------------------------------------------------------
     *
     * Basic settings and status, error
     *
     **/
private:
    int _status = 0;
    String _error;

    WiFiClientSecure _client;
    String _baseUrl;
    String _apiKey;
    String _systemPrompt;

public:
    int status() const { return _status; }
    String error() const { return _error; }

    // baseUrl ohne abschliessenden Slash, z.B.
    // "https://testingaihvj.cognitiveservices.azure.com"
    void begin(const String &url, const String &key)
    {
        _baseUrl = url;
        _apiKey = key;
        // NUR fuer Tests: keine Zertifikatspruefung.
        // Produktion: _client.setCACert(rootCaPem); statt setInsecure()
        _client.setInsecure();
    }

    void systemPrompt(const String &prompt)
    {
        _systemPrompt = prompt;

        _history.clear();
        _history.to<JsonArray>();
         
        JsonObject sys = _history.add<JsonObject>();
        sys["role"] = "system";
        sys["content"] = _systemPrompt;
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle tools and history for agentic use
     *
     **/

public:
    using ToolHandler = std::function<String(JsonObjectConst)>;

    struct Tool
    {
        String name;
        String description;
        String paramsSchema; // JSON-Schema des "parameters"-Objekts als String
        ToolHandler handler;
    };

private:
    std::vector<Tool> _tools;

    int _maxLoops = 5;
    JsonDocument _history;

    void _addHistory(const char *role, const String &content)
    {
        JsonObject msg = _history.add<JsonObject>();
        msg["role"] = role;
        msg["content"] = content;
    }

public:
    void registerTool(const String &name, const String &description, const String &paramsSchema, ToolHandler handler)
    {
        _tools.push_back({name, description, paramsSchema, handler});
    }

    // Konversation auf System-Prompt zuruecksetzen
    void clearHistory()
    {
        if (_history.size() == 0)
            return;

        _history.to<JsonArray>();

        JsonObject sys = _history.add<JsonObject>();
        sys["role"] = "system";
        sys["content"] = _systemPrompt;
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Request body and post request
     *
     **/

private:
    String _buildRequestBody(const String &model, bool stream = false)
    {
        JsonDocument req;
        req["model"] = model;
        req["stream"] = stream;
        req["messages"] = _history;

        // Provide registered tools to model
        if (!_tools.empty())
        {
            JsonArray toolsArray = req["tools"].to<JsonArray>();
            for (const auto &entry : _tools)
            {
                JsonObject tool = toolsArray.add<JsonObject>();
                tool["type"] = "function";
                tool["function"]["name"] = entry.name;
                tool["function"]["description"] = entry.description;

                JsonDocument paramsDoc;
                deserializeJson(paramsDoc, entry.paramsSchema);
                tool["function"]["parameters"] = paramsDoc;
            }
        }

        String body;
        serializeJson(req, body);

        return body;
    }

    JsonDocument _basicCompletion(const String &model)
    {
        // Prepare HTTPClient
        HTTPClient http;
        if (!http.begin(_client, _baseUrl + "/openai/v1/chat/completions"))
        {
            _error = "http.begin() failed";
            _status = -1;
            return JsonDocument();
        }
        http.addHeader("Content-Type", "application/json");
        http.addHeader("api-key", _apiKey);

        // Send Request
        String body = _buildRequestBody(model);
        _status = http.POST(body);

        // Check Response
        String response = http.getString();
        http.end();

        if (_status != 200)
        {
            _error = response;
            return JsonDocument();
        }

        if (response.isEmpty())
            return JsonDocument();

        // Handle JSON Response Object
        JsonDocument doc;
        if (deserializeJson(doc, response))
        {
            _error = "JSON parse error";
            return JsonDocument();
        }

        return doc;
    }

public:
    /*
       Agentic Chat-Loop mit tool calls.
    */
    String chat(const String &userMessage, const String &model)
    {
        _error = "";
        _status = 0;
        _addHistory("user", userMessage);

        Serial.printf("[chat] model=%s  msg=\"%.80s\"\n", model.c_str(), userMessage.c_str());

        for (int step = 0; step < _maxLoops; step++)
        {
            Serial.printf("[chat] step %d – calling _basicCompletion...\n", step);
            JsonDocument response = _basicCompletion(model);

            Serial.printf("[chat] HTTP status: %d\n", _status);
            if (_status != 200)
            {
                Serial.printf("[chat] error: %s\n", _error.c_str());
                return "";
            }

            JsonVariant choice = response["choices"][0];
            String finishReason = choice["finish_reason"].as<String>();
            Serial.printf("[chat] finish_reason: %s\n", finishReason.c_str());

            if (finishReason == "stop")
            {
                String content = choice["message"]["content"].as<String>();
                Serial.printf("[chat] done – reply length: %d chars\n", content.length());
                _addHistory("assistant", content);
                return content;
            }

            if (finishReason == "tool_calls")
            {
                Serial.println("[chat] tool_calls – not yet implemented, continuing loop");
                // TODO
                continue;
            }

            _error = String("Unexpected finish_reason: ") + finishReason;
            Serial.printf("[chat] %s\n", _error.c_str());
            return "";
        }

        _error = "Max steps (" + String(_maxLoops) + ") reached";
        Serial.printf("[chat] %s\n", _error.c_str());
        return "";
    }

    /*
       Einfaches Streaming ohne Tool-Support ohne Agentic-Loop.
    */
    void chatStream(const String &prompt, const String &model,
                    std::function<void(const String &)> onChunk,
                    std::function<void()> onEnd)
    {
        _error = "";
        _status = 0;
        _addHistory("user", prompt);

        String body = _buildRequestBody(model, true);

        HTTPClient http;
        http.begin(_client, _baseUrl + "/openai/v1/chat/completions");
        http.addHeader("Content-Type", "application/json");
        http.addHeader("api-key", _apiKey);

        _status = http.POST(body);
        if (_status != 200)
        {
            _error = http.getString();
            http.end();
            return;
        }

        WiFiClient *stream = http.getStreamPtr();
        String fullResponse;
        while (http.connected())
        {
            String line = stream->readStringUntil('\n');
            line.trim();

            if (!line.startsWith("data: "))
                continue;

            String data = line.substring(6);
            if (data == "[DONE]")
                break;

            JsonDocument chunk, filter;
            filter["choices"][0]["delta"]["content"] = true;
            deserializeJson(chunk, data, DeserializationOption::Filter(filter));

            JsonVariant content = chunk["choices"][0]["delta"]["content"];

            if (content.isNull())
                continue;

            String text = content.as<String>();
            fullResponse += text;
            onChunk(text);
        }
        _addHistory("assistant", fullResponse);
        onEnd();
        http.end();
    }
};
