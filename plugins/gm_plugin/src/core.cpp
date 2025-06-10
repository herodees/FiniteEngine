
#include <game.hpp>
#include <plugin.hpp>

namespace fin
{
    struct TextScript : public ScriptComponent
    {
        void Serialize(Entity e, msg::Var& ar) const
        {
            ar.set_item("ss", "");
        }

        void Deserialize(Entity e, msg::Var& ar)
        {
        }

        void ImguiProps(Entity e)
        {
        }

        void ImguiWorkspace(Entity e, ImGui::CanvasParams& cp)
        {
        }
    };

    struct Test : DataComponent
    {
        void Serialize(Entity e, msg::Var& ar) const
        {
            ar.set_item("ss", "");
        }

        void Deserialize(Entity e, msg::Var& ar)
        {
        }
    };

    class ExamplePlugin : public GamePlugin
    {
    public:
        ExamplePlugin()
        {
            RegisterScriptComponent<TextScript>("Text");
            RegisterComponent<Test>("test");
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
