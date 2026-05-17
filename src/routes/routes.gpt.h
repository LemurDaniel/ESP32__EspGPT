

#include <router/router.h>

namespace routes_gpt
{

    class Router : public EspWeb::Router
    {
    public:
        Router()
        {
            route("POST", "/admin/settings/gpt", set_GptSettings);
        }

    private:
        static void set_GptSettings(EspWeb::Request &request, EspWeb::Response &response);
    };

}