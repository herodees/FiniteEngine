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



    struct IGamePluginFactory
    {
        IGamePluginFactory();
        virtual IGamePlugin*       Create()     = 0;
        virtual GamePluginInfo&    Info() const = 0;
        static IGamePlugin*        _Create();
        static GamePluginInfo*     _Info();
        static IGamePluginFactory* _Root(IGamePluginFactory* to_set = nullptr);
    };



    inline IGamePluginFactory::IGamePluginFactory()
    {
        _Root(this);
    }

    inline IGamePlugin* fin::IGamePluginFactory::_Create()
    {
        IGamePluginFactory* factory = _Root();
        if (factory)
            return factory->Create();
        return nullptr;
    }

    inline GamePluginInfo* IGamePluginFactory::_Info()
    {
        IGamePluginFactory* factory = _Root();
        if (factory)
            return &factory->Info();
        return nullptr;
    }

    inline IGamePluginFactory* IGamePluginFactory::_Root(IGamePluginFactory* to_set)
    {
        static IGamePluginFactory* _root = nullptr;
        if (to_set)
            _root = to_set;
        return _root;
    }

} // namespace fin
