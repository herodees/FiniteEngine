#include "base.hpp"
#include <core/editor/imgui_control.hpp>
#include <core/scene_layer.hpp>

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
        fact.register_component<Camera>();
    }

    fin::Region<float> Base::get_bounding_box() const
    {
        fin::Region<float> bbox{_position.x, _position.y, _position.x, _position.y};
        if (Sprite::contains(_self))
        {
            auto* spr = Sprite::get(_self);
            if (spr->pack.sprite)
            {
                bbox.x1 -= spr->pack.sprite->_origina.x;
                bbox.y1 -= spr->pack.sprite->_origina.y;
                bbox.x2 = bbox.x1 + spr->pack.sprite->_source.width;
                bbox.y2 = bbox.y1 + spr->pack.sprite->_source.height;
            }
        }
        else if (Region::contains(_self))
        {
            auto* reg = Region::get(_self);
            if (!reg->_points.empty())
            {
                Vec2f min = reg->_points[0];
                Vec2f max = reg->_points[0];
                for (const auto& p : reg->_points)
                {
                    min.x = std::min(min.x, p.x);
                    min.y = std::min(min.y, p.y);
                    max.x = std::max(max.x, p.x);
                    max.y = std::max(max.y, p.y);
                }

                // Offset by position if Region is local
                bbox = fin::Region<float>(min + _position, max + _position);
            }
        }
        return bbox;
    }

    bool Base::hit_test(Vec2f pos) const
    {
        if (Region::contains(_self))
            return Region::get(_self)->hit_test(pos);

        if (get_bounding_box().contains(pos))
        {
            return true;
        }
        return false;
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
        if (r)
        {
            base.update();
        }
        return r;
    }

    void Base::update()
    {
        if (_layer)
            _layer->update(this);
    }

    bool Isometric::load(ArchiveParams& ar)
    {
        auto& iso = ar.reg.get<Isometric>(ar.entity);
        iso._a.x   = ar.data["ax"].get(0.f);
        iso._a.y   = ar.data["ay"].get(0.f);
        iso._b.x   = ar.data["bx"].get(0.f);
        iso._b.y   = ar.data["by"].get(0.f);
        return true;
    }

    bool Isometric::save(ArchiveParams& ar)
    {
        auto& iso = ar.reg.get<Isometric>(ar.entity);
        ar.data.set_item("ax", iso._a.x);
        ar.data.set_item("ay", iso._a.y);
        ar.data.set_item("bx", iso._b.x);
        ar.data.set_item("by", iso._b.y);
        return true;
    }

    bool Isometric::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Isometric>(self);
        auto  r    = ImGui::InputFloat2("A", &base._a.x);
        r |= ImGui::InputFloat2("B", &base._b.x);
        return r;
    }

    bool Collider::load(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Collider>(ar.entity);
        auto pts = ar.data.get_item("p");
        col._points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col._points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Collider::save(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Collider>(ar.entity);
        msg::Var pts;
        for (auto p : col._points)
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
        auto  r    = ImGui::PointVector("Points##cldr", &base._points, {-1, 100});
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

    bool Region::hit_test(Vec2f testPoint) const
    {
        size_t count = _points.size();
        if (!count)
            return false;

        bool inside = false;
        for (size_t i = 0, j = count - 1; i < count; j = i++)
        {
            const Vec2f& pi = _points[i];
            const Vec2f& pj = _points[j];

            if (((pi.y > testPoint.y) != (pj.y > testPoint.y)) &&
                (testPoint.x < (pj.x - pi.x) * (testPoint.y - pi.y) / (pj.y - pi.y) + pi.x))
            {
                inside = !inside;
            }
        }
        return inside;
    }

    bool Region::load(ArchiveParams& ar)
    {
        auto& col = ar.reg.get<Region>(ar.entity);
        auto  pts = ar.data.get_item("p");
        col._points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col._points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Region::save(ArchiveParams& ar)
    {
        auto&    col = ar.reg.get<Region>(ar.entity);
        msg::Var pts;
        for (auto p : col._points)
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
        auto  r    = ImGui::PointVector("Points##reg", &base._points, {-1, 100});
        return r;
    }

    bool Camera::load(ArchiveParams& ar)
    {
        auto& cam       = ar.reg.get<Camera>(ar.entity);
        cam._position.x = ar.data["x"].get(0.f);
        cam._position.y = ar.data["y"].get(0.f);
        cam._size.x     = ar.data["w"].get(0.f);
        cam._size.y     = ar.data["h"].get(0.f);
        return true;
    }

    bool Camera::save(ArchiveParams& ar)
    {
        auto& cam = ar.reg.get<Camera>(ar.entity);
        ar.data.set_item("x", cam._position.x);
        ar.data.set_item("y", cam._position.y);
        ar.data.set_item("w", cam._size.x);
        ar.data.set_item("h", cam._size.y);
        return true;
    }

    bool Camera::edit(Registry& reg, Entity self)
    {
        auto& base = reg.get<Camera>(self);
        auto  r    = ImGui::InputFloat2("Position", &base._position.x);
        r |= ImGui::InputFloat2("Size", &base._size.x);
        return r;
    }

    bool Prefab::load(ArchiveParams& ar)
    {
        auto& pf = ar.reg.get<Prefab>(ar.entity);
        pf._data = ar.data["src"];
        return true;
    }

    bool Prefab::save(ArchiveParams& ar)
    {
        auto& pf = ar.reg.get<Prefab>(ar.entity);
        ar.data.set_item("src", pf._data);
        return true;
    }
}
