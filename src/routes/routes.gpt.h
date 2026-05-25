#include <router/router.h>
#include <foundryOrchestrator.h>
#include <githubClient.h>
#include <webClient.h>
#include <LEDClient.h>

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

        static void prepareFoundry();

    private:
        static void get_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void set_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAsk(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAskStream(EspWeb::Request &request, EspWeb::Response &response);
    };

}
