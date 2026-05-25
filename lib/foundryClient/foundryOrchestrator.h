#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include "foundryClient.h"
#include "toolTypes.h"

class AzureFoundryOrchestrator
{
private:
    AzureFoundryClient _client;

    std::vector<Mcp *> _mcps;

    int _maxLoops = 5;
    String _lastResponseId;

    JsonDocument _callTools(JsonArray toolCalls)
    {
        JsonDocument merged;
        merged.to<JsonArray>();
        JsonArray results = merged.as<JsonArray>();

        for (JsonObject toolCall : toolCalls)
        {
            JsonDocument doc;
            doc.set(toolCall);
            for (Mcp *mcp : _mcps)
                mcp->executeTool(doc, results);
        }

        return merged;
    }

public:
    int status() const { return _client.status(); }
    String error() const { return _client.error(); }

    void begin(const String &url, const String &key, const String &model, const String &systemPrompt = "")
    {
        _client.begin(url, key, model, systemPrompt);
    }

    void clearHistory() { _lastResponseId = ""; }

    void registerMcp(Mcp &mcp)
    {
        _mcps.push_back(&mcp);
        for (const auto &entry : mcp.tools())
            _client.registerTool(entry.second.name, entry.second.description, entry.second.paramsSchema, entry.second.handler);
    }

    String chat(const String &userMessage)
    {
        Serial.printf("[orchestrator] msg=\"%.80s\"\n", userMessage.c_str());

        JsonDocument response = _client.complete(userMessage, _lastResponseId);

        for (int step = 0; step < _maxLoops; step++)
        {
            Serial.printf("[orchestrator] step %d\n", step);
            Serial.printf("[orchestrator] HTTP status: %d\n", _client.status());

            if (_client.status() != 200)
            {
                Serial.printf("[orchestrator] error: %s\n", _client.error().c_str());
                return "";
            }

            _lastResponseId = response["id"].as<String>();
            JsonVariant firstOutput = response["output"][0];
            String type = firstOutput["type"].as<String>();
            Serial.printf("[orchestrator] output type: %s\n", type.c_str());

            if (type == "message")
            {
                String content = firstOutput["content"][0]["text"].as<String>();
                Serial.printf("[orchestrator] done – reply length: %d chars\n", content.length());
                return content;
            }

            if (type == "function_call")
            {
                JsonDocument toolResults = _callTools(response["output"].as<JsonArray>());
                response = _client.complete(toolResults, _lastResponseId);
                continue;
            }

            Serial.printf("[orchestrator] Unexpected output type: %s\n", type.c_str());
            return "";
        }

        Serial.printf("[orchestrator] Max steps (%d) reached\n", _maxLoops);
        return "";
    }

    void chatStream(const String &prompt,
                    std::function<void(const String &)> onChunk,
                    std::function<void()> onEnd)
    {
        String result = chat(prompt);
        onChunk(result);
        onEnd();
    }
};
