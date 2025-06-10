#pragma once

#include "components.hpp"

namespace fin
{
    struct CBase : DataComponent 
    {
        Entity            _self;
        ObjectSceneLayer* _layer{};
    };

    struct CBody : DataComponent
    {
        Vec2f _previous_position;
        Vec2f _speed;
    };

    struct CPath : DataComponent
    {
        std::vector<Vec2i> _path;
    };

    struct CIsometric : DataComponent
    {
        Vec2f _a;
        Vec2f _b;
    };

    struct CCollider : DataComponent
    {
        std::vector<Vec2f> _points;
    };

    struct CSprite : DataComponent
    {
        
    };

    struct CRegion : DataComponent
    {
        std::vector<Vec2f> _points;
        bool               _local{};
    };

    struct CCamera : DataComponent
    {
        Vec2f  _position{0, 0};
        Vec2f  _size{800, 600};
        float  _zoom         = 1.0f;
        float  _zoomSpeed    = 0.1f;
        float  _moveSpeed    = 400.0f;
        float  _smoothFactor = 5.0f;
        Entity _target       = entt::null;
    };

    extern GameAPI gGameAPI;

#ifdef BUILDING_DLL

    template <typename C>
    bool RegisterBuiltinComponent(StringView, StringView, ComponentsFlags)
    {
        if (auto* nfo = gGameAPI.GetComponentInfo(gGameAPI, entt::internal::stripped_type_name<C>()))
        {
            ComponentTraits<C>::info         = static_cast<ComponentInfoStorage<C>*>(nfo);
            ComponentTraits<C>::storage      = &ComponentTraits<C>::info->set;
            ComponentTraits<C>::component_id = nfo->index;
            return true;
        }
        return false;
    }

#else

    template <typename C>
    bool RegisterBuiltinComponent(StringView name, StringView label, ComponentsFlags flags)
    {
        RegisterComponentInt<C>(name, label, flags, nullptr);
        return false;
    }

#endif

    inline void RegisterBuiltinComponents()
    {
        RegisterBuiltinComponent<CBase>("bse", "Base", 0);
        RegisterBuiltinComponent<CBody>("bdy", "Body", 0);
        RegisterBuiltinComponent<CPath>("pth", "Path", 0);
        RegisterBuiltinComponent<CIsometric>("iso", "Isometric", 0);
        RegisterBuiltinComponent<CCollider>("cld", "Collider", 0);
        RegisterBuiltinComponent<CSprite>("spr", "Sprite", 0);
        RegisterBuiltinComponent<CRegion>("reg", "Region", 0);
        RegisterBuiltinComponent<CCamera>("cam", "Camera", 0);
    }

} // namespace fin
