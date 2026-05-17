

#include <router/router.h>
#include <./foundryClient.h>

namespace routes_gpt
{

    class Router : public EspWeb::Router
    {
    public:
        Router()
        {
            route("GET", "/admin/settings/gpt", get_GptSettings);
            route("POST", "/admin/settings/gpt", set_GptSettings);

            route("POST", "/gpt/chat", post_GptAsk);
            route("POST", "/gpt/chat/stream", post_GptAskStream);
        }

    private:
        static void get_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void set_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAsk(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAskStream(EspWeb::Request &request, EspWeb::Response &response);
    };

}