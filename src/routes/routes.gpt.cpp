

#include <routes/routes.gpt.h>

namespace routes_gpt
{

    void Router::set_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {
        fs.writeJson("/gpt.settings.json", request.body.json());
        response.OK();
    }

    void Router::post_GptAsk(EspWeb::Request &request, EspWeb::Response &response)
    {

        JsonDocument settings = fs.readJson("/gpt.settings.json");

        String question = request.body.json()["msg"];

        AzureFoundryClient fc;
        fc.begin(settings["baseUrl"], settings["apiKey"]);

        String answer = fc.ask(question, settings["model"]);

        if (fc.error().length() > 0)
        {
            response.InternalServerError().text(fc.error().c_str());
        }
        else
        {
            response.OK().text(answer.c_str());
        }
    }
}