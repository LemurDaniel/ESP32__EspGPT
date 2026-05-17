

#include <router/router.h>
#include <./foundryClient.h>

namespace routes_gpt
{

    class Router : public EspWeb::Router
    {
    public:
        Router()
        {
            route("POST", "/admin/settings/gpt", set_GptSettings);
            route("POST", "/gpt/chat", post_GptAsk);
        }

    private:
        static void set_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
        static void post_GptAsk(EspWeb::Request &request, EspWeb::Response &response);
    };

}