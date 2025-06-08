#include "include.hpp"
#include "core/application.hpp"

int main(int argc, char *argv[])
{
    fin::Application app;

    if (app.OnInit(argv, argc))
    {
        app.OnDeinit(app.OnIterate());
        return EXIT_SUCCESS;
    }

    app.OnDeinit(false);
    return EXIT_FAILURE;
}
