

#include <router/router.h>
#include <foundryClient.h>
#include <githubClient.h>

namespace routes_gpt
{

    class Router : public EspWeb::Router
    {
    public:
        Router()
        {
            prepareFoundry();

            route("GET", "/admin/settings/gpt", get_GptSettings);
            route("POST", "/admin/settings/gpt", set_GptSettings);

            route("POST", "/gpt/chat", post_GptAsk);
            route("POST", "/gpt/chat/stream", post_GptAskStream);
        }

        static void prepareFoundry()
        {
            // Prepare foundry client
            JsonDocument settings = fs.readJson("/gpt.settings.json");
            _fc.begin(settings["baseUrl"], settings["apiKey"], settings["model"]);
            _gc.begin(settings["githubUser"], settings["githubToken"]);

            _fc.registerMcp(_gc);
        }

    private:
        static GithubClient _gc;
        static AzureFoundryClient _fc;

        static void get_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void set_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAsk(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAskStream(EspWeb::Request &request, EspWeb::Response &response);
    };

}