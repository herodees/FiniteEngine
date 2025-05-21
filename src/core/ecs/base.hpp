#pragma once

#include "core/atlas.hpp"
#include "component.hpp"
#include "utils/lquery.hpp"

namespace fin::ecs
{
    void register_base_components(ComponentFactory& fact);



    struct Base : lq::SpatialDatabase::Proxy, Component<Base, "bse", "Base">
    {
        static constexpr auto in_place_delete = true;

        Entity  _self;
        uint8_t _layer{0};

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };



    struct Isometric : Component<Isometric, "iso", "Isometric">
    {
        Vec2f a;
        Vec2f b;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };



    struct Collider : Component<Collider, "cld", "Collider">
    {
        std::vector<Vec2f> points;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };



    struct Sprite : Component<Sprite, "spr", "Sprite">
    {
        Atlas::Pack pack;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };



    struct Region : Component<Region, "reg", "Region">
    {
        std::vector<Vec2f> points;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };


    struct Prefab : Component<Prefab, "pfb", "Prefab">
    {
        msg::Var data;

        static bool load(ArchiveParams& ar);
        static bool save(ArchiveParams& ar);
    };
} // namespace fin::ecs
