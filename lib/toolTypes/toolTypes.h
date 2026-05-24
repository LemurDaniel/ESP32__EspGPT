#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

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
public:
    virtual std::vector<Tool> capabilities() = 0;
    virtual ~Mcp() = default;
};
