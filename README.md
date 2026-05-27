# EspGPT

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=Espressif&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-FF6000?style=for-the-badge&logo=PlatformIO&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Azure](https://img.shields.io/badge/Azure_AI_Foundry-0078D4?style=for-the-badge&logo=microsoftazure&logoColor=white)

An ESP32 turned into a standalone AI chat terminal — powered by Azure AI Foundry.

> **Learning project** — built to explore embedded HTTP clients, LLM APIs, and agentic patterns on constrained hardware. Not intended for production use.

</div>


## Overview

EspGPT runs a browser-based chat interface directly on the ESP32. Any device on the local network can open the dashboard and chat with a large language model hosted on [Azure AI Foundry](https://ai.azure.com)

---

<details>
<summary><strong>🔧 Tool Building & Creating a ToolClient</strong></summary>

### 💡 Concepts

Tools are callable functions exposed to the LLM. Each tool has a name, a description, typed parameters, and a handler. They are grouped into **ToolClients** — classes that inherit from `Mcp` and register their tools in the constructor.

The `Mcp` base class (`lib/toolTypes/tool.h`) manages a `std::map<string, Tool>` and exposes `registerTool()` for registration and `tools()` for iteration.

---

### 🛠️ Defining a Tool

```cpp
Tool myTool("tool_name", "What this tool does");
myTool.param("param1").description("What param1 is").string();
myTool.param("param2").description("What param2 is").integer().optional();
myTool.does(std::bind(&MyClient::myMethod, this, std::placeholders::_1));
registerTool(myTool);
```

**⛓️ Parameter methods** (all chainable):

| Method | Description |
|---|---|
| `.string()` | Sets type to `string` |
| `.integer()` | Sets type to `integer` |
| `.description("...")` | Adds a description for the LLM |
| `.optional()` | Makes the parameter optional |

> ℹ️ Parameters are `required` by default. Call `.optional()` to make them optional.

---

### 🤖 Creating a ToolClient

1. Inherit from `Mcp`
2. Register tools in the constructor
3. Implement handler methods with signature `String methodName(JsonDocument)`

```cpp
#include <tool.h>

class MyClient : public Mcp
{
public:
    MyClient()
    {
        Tool tHello("say_hello", "Greet a user by name");
        tHello.param("name").description("Name of the user").string();
        tHello.does(std::bind(&MyClient::sayHello, this, std::placeholders::_1));
        registerTool(tHello);
    }

    String sayHello(JsonDocument doc)
    {
        return "Hello, " + doc["name"].as<String>() + "!";
    }
};
```

</details>
