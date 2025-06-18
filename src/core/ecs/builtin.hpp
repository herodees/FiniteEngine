#pragma once

#include "api/components.hpp"
#include "core/atlas.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    namespace Sc
    {
        constexpr std::string_view Id("$id");
        constexpr std::string_view Group("$grp");
        constexpr std::string_view Name("$nme");
        constexpr std::string_view Uid("$uid");
        constexpr std::string_view Diff("$diff");
        constexpr std::string_view Class("$cls");
        constexpr std::string_view Flag("$fl");
        constexpr std::string_view Atlas("atl");
        constexpr std::string_view Sprite("spr");
    } // namespace Sc

    static constexpr size_t MaxAttachmentCount = 4;


    class ObjectSceneLayer;

    void RegisterBaseComponents(Register& fact);

    bool Serialize(const Atlas::Pack& pack, msg::Var& data);
    bool Deserialize(Atlas::Pack& pack, msg::Var& data);



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
        void          OnSerialize(ArchiveParams& ar) final;
        bool          OnDeserialize(ArchiveParams& ar) final;
        bool          OnEdit(Entity ent) final;
    };


    /// @brief Physics body component.
    /// Handles collision and rigid body serialization.
    struct CBody : IBody
    {
        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
    };


    /// @brief Pathfinding component.
    /// Stores computed navigation paths.
    struct CPath : IPath
    {
        std::vector<Vec2i> _path;

        std::span<const Vec2i> GetPath() const final;
        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
    };


    /// @brief Isometric rendering & logic marker.
    /// Enables isometric-related editing and display.
    struct CIsometric : IIsometric
    {
        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
        bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    /// @brief Polygonal collider component.
    /// Defines a shape used for collision detection and hit tests.
    struct CCollider : ICollider
    {
        std::vector<Vec2f> _points;

        std::span<const Vec2f> GetPath() const final;
        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
        bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas) final;
    };


    /// @brief Sprite component for rendering.
    /// Holds atlas data for static/dynamic sprites.
    struct CSprite : ISprite
    {
        Atlas::Pack _pack;
        void        OnSerialize(ArchiveParams& ar) final;
        bool        OnDeserialize(ArchiveParams& ar) final;
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
        void                   OnSerialize(ArchiveParams& ar) final;
        bool                   OnDeserialize(ArchiveParams& ar) final;
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

        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
    };


    /// @brief Prefab data component.
    /// Used to store serialized prefab state.
    struct CPrefab : IPrefab
    {
        msg::Var _data;

        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
    };


    /// @brief Named entity component.
    /// Holds a debug/editor-facing name.
    struct CName : IName
    {
        std::string_view _name;

        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
    };



    /// @brief Attachment entity component.
    /// Sprites rendered on top of parent entity
    struct CAttachment : IAttachment
    {
        struct Data
        {
            Vec2f       _offset;
            Atlas::Pack _sprite;
        };

        std::array<Data, MaxAttachmentCount> _items;

        int Append(const Atlas::Pack& ref, Vec2f off);
        void Remove(int idx);
        void Clear();

        void OnSerialize(ArchiveParams& ar) final;
        bool OnDeserialize(ArchiveParams& ar) final;
        bool OnEdit(Entity self) final;
    };

} // namespace fin
