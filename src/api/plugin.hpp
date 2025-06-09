#pragma once

#include "game.hpp"
#include "types.hpp"

namespace fin
{
    class IGamePlugin
    {
    public:
        virtual ~IGamePlugin() = default;

        virtual void Release()
        {
            delete this;
        };

        virtual void Save() {};
        virtual void Load() {};

        virtual void OnStart() {};
        virtual void OnStop() {};
        virtual void OnPreUpdate(float dt) {};
        virtual void OnUpdate(float dt) {};
        virtual void OnPostUpdate(float dt) {};
    };


    class GamePlugin : public IGamePlugin
    {
    public:
        ~GamePlugin() override;

        template <typename C>
        ComponentId RegisterComponent(StringView name, StringView label = StringView(), ComponentFlags flags = ComponentFlags_Default);

    private:
        std::vector<ComponentInfo*> _items;
    };



    struct IGamePluginFactory
    {
        IGamePluginFactory();
        virtual IGamePlugin*    Create()     = 0;
        virtual GamePluginInfo& Info() const = 0;

        static IGamePlugin*        InternalCreate();
        static GamePluginInfo*     InternalInfo();
        static IGamePluginFactory* InternalRoot(IGamePluginFactory* to_set = nullptr);
    };



    inline IGamePluginFactory::IGamePluginFactory()
    {
        InternalRoot(this);
    }

    inline IGamePlugin* fin::IGamePluginFactory::InternalCreate()
    {
        IGamePluginFactory* factory = InternalRoot();
        if (factory)
            return factory->Create();
        return nullptr;
    }

    inline GamePluginInfo* IGamePluginFactory::InternalInfo()
    {
        IGamePluginFactory* factory = InternalRoot();
        if (factory)
            return &factory->Info();
        return nullptr;
    }

    inline IGamePluginFactory* IGamePluginFactory::InternalRoot(IGamePluginFactory* to_set)
    {
        static IGamePluginFactory* _root = nullptr;
        if (to_set)
            _root = to_set;
        return _root;
    }

    inline GamePlugin::~GamePlugin()
    {
        for (const auto* nfo : _items)
        {
            if (nfo->owner == static_cast<const IGamePlugin*>(this))
                delete nfo;
        }
    }

    template <typename C>
    inline ComponentId GamePlugin::RegisterComponent(StringView name, StringView label, ComponentFlags flags)
    {
        static_assert(std::is_base_of_v<ScriptComponent, C>, "Component must derive from ScriptComponent");

        if (auto* info = gGameAPI.GetComponentInfo(gGameAPI, entt::internal::stripped_type_name<C>()))
        {
            throw std::runtime_error("Component already registered!");
        }
        else
        {
            auto* nfo    = new ComponentInfoStorage<C>();
            nfo->index   = uint32_t(_items.size());
            nfo->name    = entt::internal::stripped_type_name<C>();
            nfo->label   = label.empty() ? nfo->name : label;
            nfo->flags   = flags;
            nfo->owner   = static_cast<IGamePlugin*>(this);
            nfo->storage = &nfo->set;
            _items.push_back(nfo);

            ScriptTraits<C>::info         = nfo;
            ScriptTraits<C>::storage      = &nfo->set;
            ScriptTraits<C>::component_id = nfo->index;

            gGameAPI.RegisterComponentInfo(gGameAPI, nfo);
        }

        return ScriptTraits<C>::component_id;
    }

} // namespace fin
