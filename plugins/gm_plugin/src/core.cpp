
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    struct TextScript : IScriptComponent
    {
        void Init(Entity entity) override {};
        void Update(float deltaTime) override {};

        bool OnEdit(Entity ent) override
        {
            if (gImguiAPI.Button("Test", {}))
            {

            }
            return false;
        };
    };

    struct Test : IComponent
    {
    };

    class ExamplePlugin : public GamePlugin
    {
    public:
        ExamplePlugin()
        {
            RegisterComponent<TextScript>("Text", {}, ComponentsFlags_NoWorkspaceEditor);
            RegisterComponent<Test>("test");

            auto ent = gGameAPI.CreateEntity(gGameAPI.context);
            auto ent2 = gGameAPI.CreateEntity(gGameAPI.context);
            Emplace<IBase>(ent);
            IBase& base = Get<IBase>(ent);
            Test*  pbase = Find<Test>(ent);
            if (Contains<IBase>(ent))
            {
                Erase<IBase>(ent);
            }
            Emplace<Test>(ent);
            Emplace<Test>(ent2);
            Emplace<TextScript>(ent);

            View<Test, TextScript> view;
            if (view.Contains(ent))
            {
                for (Entity it : view)
                {
                    if (Contains<IBase>(ent))
                    {
                        Erase<IBase>(ent);
                    }
                }
            }

            gGameAPI.DestroyEntity(gGameAPI.context, ent);
            gGameAPI.DestroyEntity(gGameAPI.context, ent2);
            if (Contains<TextScript>(ent))
            {
                Erase<TextScript>(ent);
            }
        }

        ~ExamplePlugin() override = default;
    };



    class ExamplePluginFactory : public IGamePluginFactory
    {
    public:
        GamePlugin* Create() override
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
            info.guid        = "9088C258-FB4C-49F4-89BB-A2E008F8A388";
            return info;
        }
    };

    ExamplePluginFactory examplePluginFactoryInstance;
}
