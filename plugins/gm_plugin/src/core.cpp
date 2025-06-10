
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    struct TextScript : IScriptComponent
    {
        void Init(Entity entity) override {};
        void Update(float deltaTime) override {};
    };

    struct Test : IComponent
    {
    };

    class ExamplePlugin : public GamePlugin
    {
    public:
        ExamplePlugin()
        {
            RegisterComponent<TextScript>("Text");
            RegisterComponent<Test>("test");

            auto ent = gGameAPI.registry->Create();
            CT<IBase>::Emplace(ent);
            IBase& base = CT<IBase>::Get(ent);
            if (CT<IBase>::Contains(ent))
            {
                CT<IBase>::Erase(ent);
            }
            CT<Test>::Emplace(ent);
            CT<TextScript>::Emplace(ent);
            gGameAPI.registry->Destroy(ent);
            if (CT<TextScript>::Contains(ent))
            {
                CT<TextScript>::Erase(ent);
            }
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
