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
        auto* info = NewComponentInfo<C>(name, label, flags, static_cast<IGamePlugin*>(this));
        _items.emplace_back(info);
        gGameAPI.RegisterComponentInfo(gGameAPI.context, info);
        return info;
    }

} // namespace fin
