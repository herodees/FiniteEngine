#include "include.hpp"
#include "core/application.hpp"

int main(int argc, char *argv[])
{
    fin::application app;

    if (app.on_init(argv, argc))
    {
        app.on_deinit(app.on_iterate());
        return EXIT_SUCCESS;
    }

    app.on_deinit(false);
    return EXIT_FAILURE;
}
