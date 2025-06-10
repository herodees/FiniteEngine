#pragma once

#include "types.hpp"
#include "components.hpp"

namespace fin
{
    extern GameAPI gGameAPI;

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
        ComponentInfo* RegisterComponent(StringView id, StringView label = StringView(), ComponentsFlags flags = ComponentsFlags_Default);

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
    inline ComponentInfo* GamePlugin::RegisterComponent(StringView name, StringView label, ComponentsFlags flags)
    {
        static_assert(std::is_base_of_v<IComponent, C>, "Component must derive from IComponent");

        static_assert(std::is_default_constructible_v<C>, "Component is not default constructible, or is incomplete");

        if (auto* info = gGameAPI.GetComponentInfoByType(gGameAPI, entt::internal::stripped_type_name<C>()))
        {
            throw std::runtime_error("Component already registered!");
        }

        auto* info    = new ComponentInfoStorage<C>();
        info->name    = entt::internal::stripped_type_name<C>();
        info->id      = name;
        info->label   = label.empty() ? name : label;
        info->flags   = flags;
        info->owner   = static_cast<IGamePlugin*>(this);
        info->storage = &info->set;

        ComponentTraits<C>::info = info; 
        ComponentTraits<C>::set     = info->storage;

        gGameAPI.RegisterComponentInfo(gGameAPI, info);

        _items.emplace_back(info);
        return info;
    }

} // namespace fin
