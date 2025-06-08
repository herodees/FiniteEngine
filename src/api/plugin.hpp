#pragma once

#include "game_api.h"

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
        IGamePluginFactory()
        {
            _Root(this);
        };

        virtual ~IGamePluginFactory()        = default;
        virtual IGamePlugin*    Create()     = 0;
        virtual GamePluginInfo& Info() const = 0;

        static IGamePlugin* _Create()
        {
            IGamePluginFactory* factory = _Root();
            if (factory)
                return factory->Create();
            return nullptr;
        }

        static GamePluginInfo* _Info()
        {
            IGamePluginFactory* factory = _Root();
            if (factory)
                return &factory->Info();
            return nullptr;
        }

        static IGamePluginFactory* _Root(IGamePluginFactory* to_set = 0)
        {
            static IGamePluginFactory* _root = nullptr;
            if (to_set)
                _root = to_set;
            return _root;
        }
    };
} // namespace fin
