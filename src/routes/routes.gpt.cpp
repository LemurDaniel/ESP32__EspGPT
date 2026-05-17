

#include <routes/routes.gpt.h>

namespace routes_gpt
{

    void Router::set_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {

        JsonDocument doc;
        doc["baseUrl"] = request.body.json()["baseUrl"];
        doc["apiKey"] = request.body.json()["apiKey"];
        doc["model"] = request.body.json()["model"];

        writeJsonFile("/gpt.settings.json", doc);
        
        response.OK();
    }
}