#pragma once

#include "components.hpp"

namespace fin
{
    struct IBase : IComponent
    {
        Entity            _self;
        ObjectSceneLayer* _layer{};

        virtual Region<float> GetBoundingBox() const   = 0;
        virtual bool          HitTest(Vec2f pos) const = 0;
        virtual void          UpdateSparseGrid()       = 0;
        virtual Vec2f         GetPosition() const      = 0;
        virtual void          SetPosition(Vec2f pos)   = 0;
    };

    struct IBody : IComponent
    {
        Vec2f _previous_position;
        Vec2f _speed;
    };

    struct IPath : IComponent
    {
        virtual std::span<const Vec2f> GetPath() const = 0;
    };

    struct IIsometric : IComponent
    {
        Vec2f _a;
        Vec2f _b;
    };

    struct ICollider : IComponent
    {
        virtual std::span<const Vec2f> GetPath() const = 0;
    };

    struct ISprite : IComponent
    {
    };

    struct IRegion : IComponent
    {
        virtual std::span<const Vec2f> GetPath() const = 0;
        bool                           _local{};
    };

    struct ICamera : IComponent
    {
        Vec2f  _position{0, 0};
        Vec2f  _size{800, 600};
        float  _zoom         = 1.0f;
        float  _zoomSpeed    = 0.1f;
        float  _moveSpeed    = 400.0f;
        float  _smoothFactor = 5.0f;
        Entity _target       = entt::null;
    };

    struct IPrefab : IComponent
    {
    };

    struct IName : IComponent
    {
    };

    extern GameAPI gGameAPI;

    template <typename C>
    bool RegisterBuiltinComponent(StringView name)
    {
        if (auto* nfo = gGameAPI.GetComponentInfoById(gGameAPI, name))
        {
            ComponentTraits<C>::info    = nfo;
            ComponentTraits<C>::set     = nfo->storage;
            return true;
        }
        else
        {
            ENTT_ASSERT(false, "Component is not registered!");
        }
        return false;
    }

    inline void RegisterBuiltinComponents()
    {
        RegisterBuiltinComponent<IBase>("_");
        RegisterBuiltinComponent<IBody>("bdy");
        RegisterBuiltinComponent<IPath>("pth");
        RegisterBuiltinComponent<IIsometric>("iso");
        RegisterBuiltinComponent<ICollider>("cld");
        RegisterBuiltinComponent<ISprite>("spr");
        RegisterBuiltinComponent<IRegion>("reg");
        RegisterBuiltinComponent<ICamera>("cam");
        RegisterBuiltinComponent<IPrefab>("pfb");
        RegisterBuiltinComponent<IName>("nme");
    }

} // namespace fin
