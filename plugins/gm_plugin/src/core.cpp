
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    struct TextScript : public ScriptComponent
    {

    };

    class ExamplePlugin : public GamePlugin
    {
    public:
        ExamplePlugin()
        {
            RegisterComponent<TextScript>("Text");
        }

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
            info.name        = "Guild Master Plugin";
            info.description = "Guild Master main logic.";
            info.author      = "Martin Maceovic";
            info.version     = "1.0.0";
            return info;
        }
    };

    ExamplePluginFactory examplePluginFactoryInstance;
}
