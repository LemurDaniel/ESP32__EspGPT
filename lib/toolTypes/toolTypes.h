#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <map>

using ToolHandler = std::function<String(JsonDocument)>;

struct Tool
{
    String name;
    String description;
    String paramsSchema;
    ToolHandler handler;
};

class Mcp
{
private:
    std::map<String, Tool> _tools;

protected:
    void registerTool(const String &name, const String &description, const String &paramsSchema, ToolHandler handler)
    {
        _tools.insert({name, {name, description, paramsSchema, handler}});
    }

public:
    virtual ~Mcp() = default;

    const std::map<String, Tool> &tools() const { return _tools; }

    void executeTool(const JsonDocument toolCall, const JsonArray results)
    {
        if (toolCall["type"] != "function_call")
            return;

        String call_id = toolCall["call_id"].as<String>();
        String name = toolCall["name"].as<String>();

        JsonDocument params;
        deserializeJson(params, toolCall["arguments"].as<String>());

        const auto &tool = _tools.find(name);
        if (tool == _tools.end())
        {
            Serial.printf("[mcp] Tool not found name=%s\n", name.c_str());
            return;
        }

        Serial.printf("[mcp] executeTool name=%s\n", name.c_str());
        String result = tool->second.handler(params);

        JsonObject obj = results.add<JsonObject>();
        obj["type"] = "function_call_output";
        obj["call_id"] = call_id;
        obj["output"] = result.isEmpty() ? String("(no result)") : result;
    }
};
