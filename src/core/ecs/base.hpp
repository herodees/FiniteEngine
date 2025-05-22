#pragma once

#include "component.hpp"
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

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Registry& reg, Entity self);
    };



    struct Isometric : Component<Isometric, "iso", "Isometric">
    {
        Vec2f a;
        Vec2f b;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Registry& reg, Entity self);
    };



    struct Collider : Component<Collider, "cld", "Collider">
    {
        std::vector<Vec2f> points;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Registry& reg, Entity self);
    };



    struct Sprite : Component<Sprite, "spr", "Sprite">
    {
        Atlas::Pack pack;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Registry& reg, Entity self);
    };



    struct Region : Component<Region, "reg", "Region">
    {
        std::vector<Vec2f> points;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
        static bool edit(Registry& reg, Entity self);
    };


    struct Prefab : Component<Prefab, "pfb", "Prefab">
    {
        msg::Var data;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };
} // namespace fin::ecs
