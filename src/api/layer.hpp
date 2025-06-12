#pragma once

#include "components.hpp"
#include "types.hpp"

namespace fin
{
    constexpr int32_t TileSize(512);


    enum LayerFlags_
    {
        LayerFlags_Default  = 0,
        LayerFlags_Disabled = 1 << 0,
        LayerFlags_Hidden   = 1 << 1,
    };

    using LayerFlags = uint32_t;


    class Layer
    {
    protected:
        StringView _type;
        uint32_t   _index{uint32_t(-1)};
        Rectf      _region{};
        LayerFlags _flags{LayerFlags_Default};

    public:
        virtual ~Layer() = default;

        Vec2f ScreenToWorld(Vec2f pos) const;
        Vec2f WorldToScreen(Vec2f pos) const;

        uint32_t     GetIndex() const;
        StringView   GetType() const;
        const Rectf& GetRegion() const;
        LayerFlags   GetFlags() const;
        void         SetFlags(LayerFlags flags, bool active);
        bool         Is(LayerFlags flags) const;
        bool         IsHidden() const;
        bool         IsDisabled() const;
        void         Hide(bool b);
        void         Disable(bool b);

        virtual Vec2f            GetCursorPos() const          = 0;
        virtual StringView       GetName() const               = 0;
        virtual void             Serialize(msg::Var& ar)       = 0;
        virtual void             Deserialize(msg::Var& ar)     = 0;
        virtual void             Resize(Vec2f size)            = 0;
        virtual void             Clear()                       = 0;
        virtual void             Init()                        = 0;
        virtual void             Deinit()                      = 0;
        virtual void             Activate(const Rectf& region) = 0;
        virtual void             Update(float dt)              = 0;
        virtual void             FixedUpdate(float dt)         = 0;
        virtual void             Render(Renderer& dc)          = 0;
        virtual ObjectLayer*     Objects()                     = 0;
    };



    class ObjectLayer
    {
    public:
        virtual void             Insert(Entity ent)                         = 0;
        virtual void             Remove(Entity ent)                         = 0;
        virtual void             MoveTo(Entity ent, Vec2f pos)              = 0;
        virtual void             Move(Entity ent, Vec2f pos)                = 0;
        virtual Entity           FindAt(Vec2f position) const               = 0;
        virtual Entity           FindActiveAt(Vec2f position) const         = 0;
        virtual std::span<Vec2i> FindPath(Vec2i from, Vec2i to) const       = 0;
        virtual const SparseSet& GetObjects(bool active_only = false) const = 0;
    };



    inline Vec2f Layer::ScreenToWorld(Vec2f pos) const
    {
        return Vec2f(pos.x + _region.x, pos.y + _region.y);
    }

    inline Vec2f Layer::WorldToScreen(Vec2f pos) const
    {
        return Vec2f(pos.x - _region.x, pos.y - _region.y); 
    }

    inline uint32_t Layer::GetIndex() const
    {
        return _index;
    }

    inline StringView Layer::GetType() const
    {
        return _type;
    }

    inline const Rectf& Layer::GetRegion() const
    {
        return _region;
    }

    inline LayerFlags Layer::GetFlags() const
    {
        return _flags;
    }

    inline void Layer::SetFlags(LayerFlags flags, bool active)
    {
        if (active)
            _flags |= flags;
        else
            _flags &= ~flags;
    }

    inline bool Layer::Is(LayerFlags flags) const
    {
        return _flags & flags;
    }

    inline bool Layer::IsHidden() const
    {
        return Is(LayerFlags_Hidden);
    }

    inline bool Layer::IsDisabled() const
    {
        return Is(LayerFlags_Disabled);
    }

    inline void Layer::Hide(bool b)
    {
        SetFlags(LayerFlags_Hidden, b);
    }

    inline void Layer::Disable(bool b)
    {
        SetFlags(LayerFlags_Disabled, b);
    }


} // namespace fin
