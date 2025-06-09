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

        virtual void* Emplace(Entity ent, ComponentId cmp) = 0;
        virtual void  Remove(Entity ent, ComponentId cmp)  = 0;
        virtual void* Get(Entity ent, ComponentId cmp)     = 0;
    };


    class GamePlugin : public IGamePlugin
    {
    public:
        ~GamePlugin() override;

        template <typename C>
        ComponentId RegisterComponent(StringView name, StringView label = StringView());

        void* Emplace(Entity ent, ComponentId cmp) final;
        void* Get(Entity ent, ComponentId cmp) final;
        void  Remove(Entity ent, ComponentId cmp) final;
    private:
        std::vector<ComponentInfo> _components;
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
        for (const auto& nfo : _components)
        {
            if (!nfo.external)
                delete nfo.storage;
        }
    }

    inline void* GamePlugin::Emplace(Entity ent, ComponentId cmp)
    {
        auto it = _components[cmp].storage->emplace(ent);
        return _components[cmp].storage->get(ent);
    }

    inline void* GamePlugin::Get(Entity ent, ComponentId cmp)
    {
        return _components[cmp].storage->get(ent);
    }

    inline void GamePlugin::Remove(Entity ent, ComponentId cmp)
    {
        _components[cmp].storage->remove(ent);
    }

    template <typename C>
    inline ComponentId GamePlugin::RegisterComponent(StringView name, StringView label)
    {
        static_assert(std::is_base_of_v<ScriptComponent, C>, "Component must derive from ScriptComponent");

        if (auto* info = CGameAPI::Get().GetComponentInfo(entt::internal::stripped_type_name<C>()))
        {
            throw std::runtime_error("Component already registered!");
        }
        else
        {
            ComponentInfo& comp = _components.emplace_back();
            comp.name           = entt::internal::stripped_type_name<C>();
            comp.id             = name;
            comp.label          = label.empty() ? name : label;
            comp.size           = uint32_t(sizeof(C));
            comp.external       = false;
            comp.storage        = new entt::storage<C>();

            ScriptTraits<C>::storage = comp.storage;
            CGameAPI::Get().RegisterComponentInfo(&comp);
            ScriptTraits<C>::component_id = comp.index;
        }

        return ScriptTraits<C>::component_id;
    }

} // namespace fin
