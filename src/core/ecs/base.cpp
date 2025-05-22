#include "base.hpp"
#include <core/editor/imgui_control.hpp>

namespace fin::ecs
{
    void register_base_components(ComponentFactory& fact)
    {
        fact.register_component<Base>();
        fact.register_component<Isometric>();
        fact.register_component<Collider>();
        fact.register_component<Sprite>();
        fact.register_component<Region>();
        fact.register_component<Prefab>();
    }

    bool Base::load(ArchiveParams& ar)
    {
        auto& base       = ar.reg.get<Base>(ar.entity);
        base._self       = ar.entity;
        base._position.x = ar.data["x"].get(0.f);
        base._position.y = ar.data["y"].get(0.f);

        return true;
    }

    bool Base::save(ArchiveParams& ar)
    {
        auto& base = ar.reg.get<Base>(ar.entity);
        ar.data.set_item("x", base._position.x);
        ar.data.set_item("y", base._position.y);

        return true;
    }

    bool Base::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Base>(self);
        auto r = ImGui::InputFloat2("Position", &base._position.x);

        return r;
    }

    bool Isometric::load(ArchiveParams& ar)
    {
        auto& iso = ar.reg.get<Isometric>(ar.entity);
        iso.a.x   = ar.data["ax"].get(0.f);
        iso.a.y   = ar.data["ay"].get(0.f);
        iso.b.x   = ar.data["bx"].get(0.f);
        iso.b.y   = ar.data["by"].get(0.f);
        return true;
    }

    bool Isometric::save(ArchiveParams& ar)
    {
        auto& iso = ar.reg.get<Isometric>(ar.entity);
        ar.data.set_item("ax", iso.a.x);
        ar.data.set_item("ay", iso.a.y);
        ar.data.set_item("bx", iso.b.x);
        ar.data.set_item("by", iso.b.y);
        return true;
    }

    bool Isometric::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Isometric>(self);
        auto  r    = ImGui::InputFloat2("A", &base.a.x);
        r |= ImGui::InputFloat2("B", &base.b.x);
        return r;
    }

    bool Collider::load(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Collider>(ar.entity);
        auto pts = ar.data.get_item("p");
        col.points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col.points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Collider::save(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Collider>(ar.entity);
        msg::Var pts;
        for (auto p : col.points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
        return true;
    }

    bool Collider::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Collider>(self);
        auto  r    = ImGui::PointVector("Points##cldr", &base.points, {-1, 100});
        return r;
    }

    bool Sprite::load(ArchiveParams& ar)
    {
        auto& spr = ar.reg.get<Sprite>(ar.entity);
        auto  src = ar.data["src"];
        auto  sp = ar.data["spr"];
        spr.pack = Atlas::load_shared(src.str(), sp.str());
        return true;
    }

    bool Sprite::save(ArchiveParams& ar)
    {
        auto& spr = ar.reg.get<Sprite>(ar.entity);
        if (spr.pack.atlas)
        {
            ar.data.set_item("src", spr.pack.atlas->get_path());
            if (spr.pack.sprite)
            {
                ar.data.set_item("spr", spr.pack.sprite->_name);
            }
        }
        return true;
    }

    bool Sprite::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Sprite>(self);
        auto  r    = ImGui::SpriteInput("Sprite##spr", &base.pack);

        return r;
    }

    bool Region::load(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Region>(ar.entity);
        auto  pts = ar.data.get_item("p");
        col.points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col.points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Region::save(ArchiveParams& ar)
    {
        auto&    col = ar.reg.get<Region>(ar.entity);
        msg::Var pts;
        for (auto p : col.points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
        return true;
    }

    bool Region::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Region>(self);
        auto  r    = ImGui::PointVector("Points##reg", &base.points, {-1, 100});
        return r;
    }

    bool Prefab::load(ArchiveParams& ar)
    {
        auto& pf = ar.reg.get<Prefab>(ar.entity);
        pf.data = ar.data["src"];
        return true;
    }

    bool Prefab::save(ArchiveParams& ar)
    {
        auto& pf = ar.reg.get<Prefab>(ar.entity);
        ar.data.set_item("src", pf.data);
        return true;
    }
}
