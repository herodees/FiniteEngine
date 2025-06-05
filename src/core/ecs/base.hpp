#pragma once

#include "component.hpp"
#include "behavior.hpp"
#include "core/atlas.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class ObjectSceneLayer;
}

namespace fin::ecs
{
    void RegisterBaseComponents(ComponentFactory& fact);


    struct Base : lq::SpatialDatabase::Proxy, Component<Base, "_", "Base">
    {
        static constexpr auto in_place_delete = true;

        Entity  _self;
        ObjectSceneLayer* _layer{};

        fin::Region<float> GetBoundingBox() const;
        bool               HitTest(Vec2f pos) const;
        void               UpdateSparseGrid();

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };



    struct Body : Component<Body, "bdy", "Body">
    {
        Vec2f       _previous_position;
        Vec2f       _speed{};

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };



    struct Path : Component<Path, "pth", "Path">
    {
        std::vector<Vec2i> _path;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };



    struct Isometric : Component<Isometric, "iso", "Isometric">
    {
        Vec2f _a;
        Vec2f _b;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
        static bool ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self);
    };



    struct Collider : Component<Collider, "cld", "Collider">
    {
        std::vector<Vec2f> _points;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
        static bool ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self);
    };



    struct Sprite : Component<Sprite, "spr", "Sprite">
    {
        Atlas::Pack pack;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };



    struct Region : Component<Region, "reg", "Region">
    {
        std::vector<Vec2f> _points;
        bool               _local{};

        bool HitTest(Vec2f pos) const;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
        static bool ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self);
    };


    struct Camera : Component<Camera, "cam", "Camera">
    {
        Vec2f  _position{0, 0};
        Vec2f  _size{800, 600};
        float  _zoom         = 1.0f;
        float  _zoomSpeed    = 0.1f;
        float  _moveSpeed    = 400.0f;
        float  _smoothFactor = 5.0f;
        Entity _target       = entt::null;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };



    struct Prefab : Component<Prefab, "pfb", "Prefab">
    {
        msg::Var _data;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
    };



    struct Name : Component<Name, "nme", "Named Object">
    {
        std::string_view _name;

        static bool Load(ArchiveParams& ar);
        static bool Save(ArchiveParams& ar);
        static bool ImguiProps(Entity self);
    };

} // namespace fin::ecs
