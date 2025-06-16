#pragma once

#include "components.hpp"

namespace fin
{
    struct IBase : IComponent
    {
        Entity       _self;
        ObjectLayer* _layer{};

        virtual Region<float> GetBoundingBox() const   = 0;
        virtual bool          HitTest(Vec2f pos) const = 0;
        virtual void          UpdateSparseGrid()       = 0;
        virtual Vec2f         GetPosition() const      = 0;
        virtual void          SetPosition(Vec2f pos)   = 0;

        inline static std::string_view CID = "_";
    };

    struct IBody : IComponent
    {
        Vec2f _previous_position;
        Vec2f _speed;

        inline static std::string_view CID = "bdy";
    };

    struct IPath : IComponent
    {
        virtual std::span<const Vec2i> GetPath() const = 0;

        inline static std::string_view CID = "pth";
    };

    struct IIsometric : IComponent
    {
        Vec2f _a;
        Vec2f _b;

        inline static std::string_view CID = "iso";
    };

    struct ICollider : IComponent
    {
        virtual std::span<const Vec2f> GetPath() const = 0;

        inline static std::string_view CID = "cld";
    };

    struct ISprite : IComponent
    {
        inline static std::string_view CID = "spr";
    };

    struct IRegion : IComponent
    {
        virtual std::span<const Vec2f> GetPath() const = 0;
        bool                           _local{};

        inline static std::string_view CID = "reg";
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

        inline static std::string_view CID = "cam";
    };

    struct IPrefab : IComponent
    {
        inline static std::string_view CID = "pfb";
    };

    struct IName : IComponent
    {
        inline static std::string_view CID = "nme";
    };

    struct IAttachment : IComponent
    {
        inline static std::string_view CID = "att";
    };



    extern GameAPI gGameAPI;

    inline bool RegisterBuiltinComponents()
    {
        bool ret = true;
        ret &= LoadBuiltinComponent<IBase>(IBase::CID);
        ret &= LoadBuiltinComponent<IBody>(IBody::CID);
        ret &= LoadBuiltinComponent<IPath>(IPath::CID);
        ret &= LoadBuiltinComponent<IIsometric>(IIsometric::CID);
        ret &= LoadBuiltinComponent<ICollider>(ICollider::CID);
        ret &= LoadBuiltinComponent<ISprite>(ISprite::CID);
        ret &= LoadBuiltinComponent<IRegion>(IRegion::CID);
        ret &= LoadBuiltinComponent<ICamera>(ICamera::CID);
        ret &= LoadBuiltinComponent<IAttachment>(IAttachment::CID);
        ret &= LoadBuiltinComponent<IPrefab>(IPrefab::CID);
        ret &= LoadBuiltinComponent<IName>(IName::CID);
        return ret;
    }

} // namespace fin
