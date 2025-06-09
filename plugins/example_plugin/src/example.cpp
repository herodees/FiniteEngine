
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    class ExamplePlugin : public GamePlugin
    {
    public:
        ExamplePlugin()  = default;
        ~ExamplePlugin() override = default;
    };



    class ExamplePluginFactory : public IGamePluginFactory
    {
    public:
        IGamePlugin* Create() override
        {
            return new ExamplePlugin();
        }

        GamePluginInfo& Info() const override
        {
            static GamePluginInfo info{};
            info.name        = "Finite Game Plugin";
            info.description = "A plugin for the Finite game engine.";
            info.author      = "Finite Team";
            info.version     = "1.0.0";
            return info;
        }
    };

    ExamplePluginFactory examplePluginFactoryInstance;
}
