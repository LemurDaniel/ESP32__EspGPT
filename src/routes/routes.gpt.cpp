

#include <routes/routes.gpt.h>

namespace routes_gpt
{
    static GithubClient _gc;
    static WebClient _wc;
    static LEDClient _led;
    static AzureFoundryOrchestrator _fo;

    void Router::prepareFoundry()
    {
        JsonDocument settings = fs.readJson("/gpt.settings.json");
        _fo.begin(settings["baseUrl"], settings["apiKey"], settings["model"], settings["systemPrompt"]);
        _fo.registerMcp(
            _led.begin(GPIO_NUM_2)
        );
        _fo.registerMcp(
            _gc.begin(settings["githubUser"], settings["githubToken"])
        );
        _fo.registerMcp(
            _wc.begin()
        );
    }

    void Router::set_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {
        fs.writeJson("/gpt.settings.json", request.body.json());
        prepareFoundry(); // Update foundry on changes
        response.OK();
    }

    void Router::get_GptSettings(EspWeb::Request &request, EspWeb::Response &response)
    {
        JsonDocument settings = fs.readJson("/gpt.settings.json");
        response.json(settings).OK();
    }

    void Router::post_GptAsk(EspWeb::Request &request, EspWeb::Response &response)
    {

        // Ask Question
        String question = request.body.json()["msg"];
        String answer = _fo.chat(question);

        if (_fo.error().length() > 0)
        {
            answer = _fo.error();
        }

        // This is just to make frontend less complicated. Returns like a stream, but not really :D
        response.beginStream();
        response.sendChunk(answer.c_str());
        response.endStream();
    }

    void Router::post_GptAskStream(EspWeb::Request &request, EspWeb::Response &response)
    {

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
        _fo.chatStream(question, onChunkCallback, onEndCallback);
    }
}