#pragma once

#include "behavior.hpp"
#include "api/components.hpp"
#include "core/atlas.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class ObjectSceneLayer;

    void RegisterBaseComponents(Register& fact);



    struct CBase : lq::SpatialDatabase::Proxy, IBase
    {
        static constexpr auto in_place_delete = true;

        Region<float> GetBoundingBox() const final;
        bool          HitTest(Vec2f pos) const final;
        void          UpdateSparseGrid() final;
        Vec2f         GetPosition() const final;
        void          SetPosition(Vec2f pos) final;
        void          OnSerialize(ArchiveParams2& ar) final;
        bool          OnDeserialize(ArchiveParams2& ar) final;
        bool          OnEdit(Entity ent) final;
    };


    struct CBody : IBody
    {
        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };


    struct CPath : IPath
    {
        std::vector<Vec2i> _path;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };


    struct CIsometric : IIsometric 
    {
        void        OnSerialize(ArchiveParams2& ar) final;
        bool        OnDeserialize(ArchiveParams2& ar) final;
        bool        OnEdit(Entity self) final;
        bool        OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    struct CCollider : ICollider
    {
        std::vector<Vec2f> _points;

        void        OnSerialize(ArchiveParams2& ar) final;
        bool        OnDeserialize(ArchiveParams2& ar) final;
        bool        OnEdit(Entity self) final;
        bool        OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    struct CSprite : ISprite
    {
        Atlas::Pack _pack;
        void        OnSerialize(ArchiveParams2& ar) final;
        bool        OnDeserialize(ArchiveParams2& ar) final;
        bool        OnEdit(Entity self) final;
    };


    struct CRegion : IRegion
    {
        std::vector<Vec2f> _points;
        bool               _local{};

        bool                   HitTest(Vec2f pos) const;
        std::span<const Vec2f> GetPath() const final;
        void                   OnSerialize(ArchiveParams2& ar) final;
        bool                   OnDeserialize(ArchiveParams2& ar) final;
        bool                   OnEdit(Entity self) final;
        bool                   OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    struct CCamera : ICamera
    {
        Vec2f  _position{0, 0};
        Vec2f  _size{800, 600};
        float  _zoom         = 1.0f;
        float  _zoomSpeed    = 0.1f;
        float  _moveSpeed    = 400.0f;
        float  _smoothFactor = 5.0f;
        Entity _target       = entt::null;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };


    struct CPrefab : IPrefab
    {
        msg::Var _data;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
    };


    struct CName : IName
    {
        std::string_view _name;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };

} // namespace fin
