#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <map>

using ToolHandler = std::function<String(JsonDocument)>;

class Parameter
{
private:
    std::string _name;
    JsonDocument &_doc;

public:
    Parameter(JsonDocument &doc, std::string name) : _doc(doc), _name(name) {
        _doc["required"].add(name);
    }

    Parameter &description(const std::string description)
    {
        _doc["properties"][_name]["description"] = description;
        return *this;
    }

    Parameter &optional()
    {
        _doc["required"].remove(_name);
        return *this;
    }

    Parameter &string()
    {
        _doc["properties"][_name]["type"] = "string";
        return *this;
    }

    Parameter &integer()
    {
        _doc["properties"][_name]["type"] = "integer";
        return *this;
    }
};

class Tool
{

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle Schema
     *
     **/
private:
    JsonDocument _schema;

public:
    std::string schema() const
    {
        std::string result;
        serializeJson(_schema, result);
        return result;
    }

    Parameter param(const std::string &name)
    {
        return Parameter(_schema, name);
    }
    /*-------------------------------------------------------------------------------------------------
     *
     * Handle Tool and handler
     *
     **/

private:
    std::string _name;
    std::string _description;
    ToolHandler _handler;

public:
    Tool(const std::string &name, const std::string &description) : _name(name), _description(description)
    {
        _schema["type"] = "object";
        _schema["properties"].to<JsonObject>();
    }

    std::string name() const { return _name; }
    std::string description() const { return _description; }

    Tool &does(ToolHandler handler)
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
