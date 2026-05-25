#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <map>

using ToolHandler = std::function<String(JsonDocument)>;

class Schema
{
private:
    JsonDocument _doc;

public:
    Schema()
    {
        _doc["type"] = "object";
        _doc["properties"].to<JsonObject>();
    }

    Schema &string(const std::string &name, const std::string &description = "")
    {
        _doc["properties"][name]["type"] = "string";
        if (!description.empty())
            _doc["properties"][name]["description"] = description;
        return *this;
    }

    Schema &required(const std::string &name)
    {
        _doc["required"].add(name);
        return *this;
    }

    String build() const
    {
        String out;
        serializeJson(_doc, out);
        return out;
    }
};

class Tool
{
private:
    std::string _name;
    std::string _description;
    Schema _schema;
    ToolHandler _handler;

public:
    Tool(const std::string &name, const std::string &description) : _name(name), _description(description) {}

    std::string name() const { return _name; }
    std::string description() const { return _description; }
    String schemaJson() const { return _schema.build(); }

    Tool &withSchema(Schema schema)
    {
        _schema = schema;
        return *this;
    }

    Tool &onCall(ToolHandler handler)
    {
        _handler = handler;
        return *this;
    }

    String call(JsonDocument params) const
    {
        return _handler(params);
    }
};

class Mcp
{
private:
    std::map<std::string, Tool> _tools;

protected:
    void registerTool(Tool tool)
    {
        _tools.insert({tool.name(), tool});
    }

public:
    virtual ~Mcp() = default;
    const std::map<std::string, Tool> &tools() const { return _tools; }
};
