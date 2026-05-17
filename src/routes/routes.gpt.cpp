

#include <routes/routes.gpt.h>

namespace routes_gpt
{

    void Router::set_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {
        fs.writeJson("/gpt.settings.json", request.body.json());
        response.OK();
    }

    void Router::get_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {
        JsonDocument settings = fs.readJson("/gpt.settings.json");
        response.json(settings).OK();
    }

    void Router::post_GptAsk(EspWeb::Request &request, EspWeb::Response &response)
    {

        // Prepare foundry client
        AzureFoundryClient fc;
        JsonDocument settings = fs.readJson("/gpt.settings.json");
        fc.begin(settings["baseUrl"], settings["apiKey"]);

        // Ask Question
        String question = request.body.json()["msg"];
        String answer = fc.chat(question, settings["model"]);

        if (fc.error().length() > 0)
        {
            answer = fc.error();
        }

        // This is just to make frontend less complicated. Returns like a stream, but not really :D
        response.beginStream();
        response.sendChunk(answer.c_str());
        response.endStream();
    }

    void Router::post_GptAskStream(EspWeb::Request &request, EspWeb::Response &response)
    {

        // Prepare foundry client
        AzureFoundryClient fc;
        JsonDocument settings = fs.readJson("/gpt.settings.json");
        fc.begin(settings["baseUrl"], settings["apiKey"]);

        const auto onChunkCallback = [&response](const String &chunk)
        {
            response.sendChunk(chunk.c_str());
        };
        const auto onEndCallback = [&response]()
        {
            // End stream
            response.endStream();
        };

        // set streaming
        response.beginStream();
        response.setTimeout(60);

        // ask question
        String question = request.body.json()["msg"];
        fc.chatStream(question, settings["model"], onChunkCallback, onEndCallback);
    }
}