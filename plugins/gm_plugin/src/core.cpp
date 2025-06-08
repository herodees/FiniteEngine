
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    struct TextScript : public ScriptComponent
    {

    };

    class ExamplePlugin : public IGamePlugin
    {
    public:
        ExamplePlugin()
        {
            auto& api = CGameAPI::Get();
            api.RegisterComponent<TextScript>();
            api.GetComponent<TextScript>(1);
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
