#pragma once

#include <Arduino.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include "toolTypes.h"

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
    String _model;
    String _systemPrompt;

public:
    int status() const { return _status; }
    String error() const { return _error; }

    // baseUrl ohne abschliessenden Slash, z.B.
    // "https://testingaihvj.cognitiveservices.azure.com"
    void begin(const String &url, const String &key, const String &model)
    {
        _baseUrl = url;
        _apiKey = key;
        _model = model;
        // NUR fuer Tests: keine Zertifikatspruefung.
        // Produktion: _client.setCACert(rootCaPem); statt setInsecure()
        _client.setInsecure();

        this->registerTool("call_url", "Call a URL via HTTP Client on C++",
                           R"({"type":"object","properties":{"Url":{"type":"string","description":"The URL to call"}},"required":["Url"]})",
                           [this](JsonDocument doc) -> String
                           {
                               HTTPClient http;
                               if (!http.begin(_client, doc["Url"]))
                               {
                                   _error = "http.begin() failed";
                                   _status = -1;
                                   return String();
                               }

                               size_t status = http.GET();
                               String response = http.getString();
                               http.end();

                               return response;
                           });
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

private:
    std::vector<Mcp *> _mcps;
    std::map<String, Tool> _tools;

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
        Tool tool;
        tool.name = name;
        tool.description = description;
        tool.paramsSchema = paramsSchema;
        tool.handler = handler;
        _tools.insert({name, tool});
    }

    void registerMcp(Mcp &mcp)
    {
        _mcps.push_back(&mcp);
        for (Tool &tool : mcp.capabilities())
            registerTool(tool.name, tool.description, tool.paramsSchema, tool.handler);
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
                tool["function"]["name"] = entry.first;
                tool["function"]["description"] = entry.second.description;

                JsonDocument paramsDoc;
                deserializeJson(paramsDoc, entry.second.paramsSchema);
                tool["function"]["parameters"] = paramsDoc;
            }
        }

        String body;
        serializeJson(req, body);

        return body;
    }

    void _callTools(JsonArray toolCalls)
    {
        for (JsonObject toolCall : toolCalls)
        {
            String id = toolCall["id"].as<String>();
            String name = toolCall["function"]["name"].as<String>();
            String params = toolCall["function"]["arguments"].as<String>();

            Serial.printf("[chat] tool_call id=%s name=%s\n", id.c_str(), name.c_str());

            String result;
            const auto tool = _tools.find(name);
            if (tool != _tools.end())
            {
                JsonDocument docParams;
                deserializeJson(docParams, params);
                result = tool->second.handler(docParams);
            }
            else
            {
                result = "Error: unknown tool " + name;
            }

            JsonObject toolMsg = _history.add<JsonObject>();
            toolMsg["role"] = "tool";
            toolMsg["tool_call_id"] = id;
            toolMsg["content"] = result.isEmpty() ? String("(no result)") : result;
        }
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
    String chat(const String &userMessage)
    {
        _error = "";
        _status = 0;
        _addHistory("user", userMessage);

        Serial.printf("[chat] model=%s  msg=\"%.80s\"\n", _model.c_str(), userMessage.c_str());

        for (int step = 0; step < _maxLoops; step++)
        {
            Serial.printf("[chat] step %d – calling _basicCompletion...\n", step);
            JsonDocument response = _basicCompletion(_model);

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
                // Add assistant message with tool_calls to history.
                // Azure rejects null content, so coerce it to empty string.
                JsonObject assistantMsg = _history.add<JsonObject>();
                assistantMsg.set(choice["message"]);
                if (assistantMsg["content"].isNull())
                    assistantMsg["content"] = "";
                JsonArray toolCalls = choice["message"]["tool_calls"].as<JsonArray>();
                _callTools(toolCalls);
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
    void chatStream(const String &prompt,
                    std::function<void(const String &)> onChunk,
                    std::function<void()> onEnd)
    {
        _error = "";
        _status = 0;
        _addHistory("user", prompt);

        String body = _buildRequestBody(_model, true);

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
