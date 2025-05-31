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
    void register_base_components(ComponentFactory& fact);


    struct Base : lq::SpatialDatabase::Proxy, Component<Base, "_", "Base">
    {
        static constexpr auto in_place_delete = true;

        Entity  _self;
        ObjectSceneLayer* _layer{};

        fin::Region<float> get_bounding_box() const;
        bool               hit_test(Vec2f pos) const;
        void               update();

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
    };



    struct Body : Component<Body, "bdy", "Body">
    {
        Vec2f       _previous_position;
        Vec2f       _speed{};

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
    };



    struct Path : Component<Path, "pth", "Path">
    {
        std::vector<Vec2i> _path;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
    };



    struct Isometric : Component<Isometric, "iso", "Isometric">
    {
        Vec2f _a;
        Vec2f _b;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
        static bool edit_canvas(ImGui::CanvasParams& canvas, Entity self);
    };



    struct Collider : Component<Collider, "cld", "Collider">
    {
        std::vector<Vec2f> _points;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
        static bool edit_canvas(ImGui::CanvasParams& canvas, Entity self);
    };



    struct Sprite : Component<Sprite, "spr", "Sprite">
    {
        Atlas::Pack pack;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
    };



    struct Region : Component<Region, "reg", "Region">
    {
        std::vector<Vec2f> _points;
        bool               _local{};

        bool hit_test(Vec2f pos) const;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
        static bool edit_canvas(ImGui::CanvasParams& canvas, Entity self);
    };



    struct Camera : Component<Camera, "cam", "Camera">
    {
        Vec2f _position;
        Vec2f _size;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Entity self);
    };



    struct Prefab : Component<Prefab, "pfb", "Prefab">
    {
        msg::Var _data;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };


    struct Script : Component<Script, "scr", "Script">
    {
        Script() = default;
        ~Script();
        IBehaviorScript* get_script(std::string_view name) const;
        void             add_script(IBehaviorScript* scr);
        void             remove_script(IBehaviorScript* scr);
        IBehaviorScript* _script{};

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };
} // namespace fin::ecs
