#pragma once

#include "api/components.hpp"
#include "behavior.hpp"
#include "core/atlas.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class ObjectSceneLayer;

    void RegisterBaseComponents(Register& fact);


    /// @brief Base spatial component.
    /// Provides position, bounding box, and sparse grid integration.
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


    /// @brief Physics body component.
    /// Handles collision and rigid body serialization.
    struct CBody : IBody
    {
        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };


    /// @brief Pathfinding component.
    /// Stores computed navigation paths.
    struct CPath : IPath
    {
        std::vector<Vec2i> _path;

        std::span<const Vec2i> GetPath() const final;
        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };


    /// @brief Isometric rendering & logic marker.
    /// Enables isometric-related editing and display.
    struct CIsometric : IIsometric
    {
        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
        bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    /// @brief Polygonal collider component.
    /// Defines a shape used for collision detection and hit tests.
    struct CCollider : ICollider
    {
        std::vector<Vec2f> _points;

        std::span<const Vec2f> GetPath() const final;
        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
        bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    /// @brief Sprite component for rendering.
    /// Holds atlas data for static/dynamic sprites.
    struct CSprite : ISprite
    {
        Atlas::Pack _pack;
        void        OnSerialize(ArchiveParams2& ar) final;
        bool        OnDeserialize(ArchiveParams2& ar) final;
        bool        OnEdit(Entity self) final;
    };


    /// @brief Region shape component.
    /// Represents an editable polygon area, optionally local.
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


    /// @brief Camera control component.
    /// Manages viewport position, zoom, and movement smoothing.
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


    /// @brief Prefab data component.
    /// Used to store serialized prefab state.
    struct CPrefab : IPrefab
    {
        msg::Var _data;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
    };


    /// @brief Named entity component.
    /// Holds a debug/editor-facing name.
    struct CName : IName
    {
        std::string_view _name;

        void OnSerialize(ArchiveParams2& ar) final;
        bool OnDeserialize(ArchiveParams2& ar) final;
        bool OnEdit(Entity self) final;
    };

} // namespace fin
