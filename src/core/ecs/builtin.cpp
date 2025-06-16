#include "builtin.hpp"
#include "core/editor/imgui_control.hpp"
#include "core/scene_layer_object.hpp"

namespace fin
{
    template <typename T>
    static void RegBuiltin(StringView name, StringView label, ComponentsFlags flags)
    {
        gGameAPI.RegisterComponentInfo(gGameAPI.context, NewComponentInfo<T>(name, label, flags));
    }

    void RegisterBaseComponents(Register& fact)
    {
        RegBuiltin<CBase>(CBase::CID, "Base", ComponentsFlags_NoWorkspaceEditor);
        RegBuiltin<CName>(CName::CID, "Name", ComponentsFlags_NoWorkspaceEditor | ComponentsFlags_NoPrefab);
        RegBuiltin<CIsometric>(CIsometric::CID, "Isometric", ComponentsFlags_Default);
        RegBuiltin<CBody>(CBody::CID, "Body", ComponentsFlags_NoWorkspaceEditor);
        RegBuiltin<CPath>(CPath::CID, "Path", ComponentsFlags_NoWorkspaceEditor);
        RegBuiltin<CCollider>(CCollider::CID, "Collider", ComponentsFlags_Default);
        RegBuiltin<CSprite>(CSprite::CID, "Sprite", ComponentsFlags_NoWorkspaceEditor);
        RegBuiltin<CAttachment>(CAttachment::CID, "Attachment", ComponentsFlags_Default);
        RegBuiltin<CRegion>(CRegion::CID, "Region", ComponentsFlags_Default);
        RegBuiltin<CCamera>(CCamera::CID, "Camera", ComponentsFlags_NoWorkspaceEditor);
        RegBuiltin<CPrefab>(CPrefab::CID,
                            "Prefab",
                            ComponentsFlags_Private | ComponentsFlags_NoWorkspaceEditor | ComponentsFlags_NoEditor);
    }

    Vec2f CBase::GetPosition() const
    {
        return _position;
    }

    void CBase::SetPosition(Vec2f pos)
    {
        _position = pos;
    }

    fin::Region<float> CBase::GetBoundingBox() const
    {
        fin::Region<float> bbox{_position.x, _position.y, _position.x, _position.y};
        if (ComponentTraits<CSprite>::Contains(_self))
        {
            auto& spr = ComponentTraits<CSprite>::Get(_self);
            if (spr._pack.sprite)
            {
                bbox.x1 -= spr._pack.sprite->_origina.x;
                bbox.y1 -= spr._pack.sprite->_origina.y;
                bbox.x2 = bbox.x1 + spr._pack.sprite->_source.width;
                bbox.y2 = bbox.y1 + spr._pack.sprite->_source.height;
            }
        }
        else if (ComponentTraits<CRegion>::Contains(_self))
        {
            auto& reg = ComponentTraits<CRegion>::Get(_self);
            if (!reg._points.empty())
            {
                Vec2f min = reg._points[0];
                Vec2f max = reg._points[0];
                for (const auto& p : reg._points)
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

    void CBase::UpdateSparseGrid()
    {
        if (_layer)
            static_cast<ObjectSceneLayer*>(_layer)->Update(this);
    }

    bool CBase::HitTest(Vec2f pos) const
    {
        if (ComponentTraits<CRegion>::Contains(_self))
            return ComponentTraits<CRegion>::Get(_self).HitTest(pos);

        if (GetBoundingBox().contains(pos))
        {
            return true;
        }
        return false;
    }

    void CBase::OnSerialize(ArchiveParams2& ar)
    {
        ar.data.set_item("x", _position.x);
        ar.data.set_item("y", _position.y);
    }

    bool CBase::OnDeserialize(ArchiveParams2& ar)
    {
        _self       = ar.entity;
        _position.x = ar.data["x"].get(0.f);
        _position.y = ar.data["y"].get(0.f);
        return true;
    }

    bool CBase::OnEdit(Entity ent)
    {
        auto r = ImGui::InputFloat2("Position", &_position.x);
        if (r)
        {
            UpdateSparseGrid();
        }
        return r;
    }



    
    bool CIsometric::OnDeserialize(ArchiveParams2& ar)
    {
        _a.x  = ar.data["ax"].get(0.f);
        _a.y  = ar.data["ay"].get(0.f);
        _b.x  = ar.data["bx"].get(0.f);
        _b.y  = ar.data["by"].get(0.f);
        return true;
    }

    void CIsometric::OnSerialize(ArchiveParams2& ar)
    {
        ar.data.set_item("ax", _a.x);
        ar.data.set_item("ay", _a.y);
        ar.data.set_item("bx", _b.x);
        ar.data.set_item("by", _b.y);
    }

    bool CIsometric::OnEdit(Entity ent)
    {
        auto  r    = ImGui::InputFloat2("A", &_a.x);
        r |= ImGui::InputFloat2("B", &_b.x);
        return r;
    }

    bool CIsometric::OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
    {
        bool    ret  = false;

        ImVec2& a    = (ImVec2&)_a;
        ImVec2& b    = (ImVec2&)_b;
        ImVec2  snap{1, 1};
        std::swap(snap, canvas.snap_grid);

        ret |= canvas.DragPoint(b, &_b, 5);
        ret |= canvas.DragPoint(a, &_a, 5);

        ImGui::GetWindowDrawList()->AddLine(canvas.WorldToScreen(a), canvas.WorldToScreen(b), IM_COL32(255, 255, 255, 255), 2);
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(a), 5, IM_COL32(0, 255, 0, 255));
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(b), 5, IM_COL32(255, 0, 0, 255));

        std::swap(snap, canvas.snap_grid);
        return ret;
    }




    bool CCollider::OnDeserialize(ArchiveParams2& ar)
    {
        auto  pts = ar.data.get_item("p");
        _points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            _points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    std::span<const Vec2f> CCollider::GetPath() const
    {
        return _points;
    }

    void CCollider::OnSerialize(ArchiveParams2& ar)
    {
        msg::Var pts;
        for (auto p : _points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
    }

    bool CCollider::OnEdit(Entity ent)
    {
        auto  r    = ImGui::PointVector("Points##cldr", &_points, {-1, 100});
        return r;
    }

    bool CCollider::OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
    {
        bool   ret  = false;

        ImVec2 snap{1, 1};
        std::swap(snap, canvas.snap_grid);
        for (size_t i = 0; i < _points.size(); ++i)
        {
            auto& p1 = (ImVec2&)_points[i];
            auto& p2 = (ImVec2&)_points[(i + 1) % _points.size()];

            ret |= canvas.DragPoint(p1, &_points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }
        std::swap(snap, canvas.snap_grid);
        return ret;
    }




    
    bool CSprite::OnDeserialize(ArchiveParams2& ar)
    {
        auto  src = ar.data["src"];
        auto  sp  = ar.data["spr"];
        _pack     = Atlas::load_shared(src.str(), sp.str());
        return true;
    }

    void CSprite::OnSerialize(ArchiveParams2& ar)
    {
        if (_pack.atlas)
        {
            ar.data.set_item("src", _pack.atlas->get_path());
            if (_pack.sprite)
            {
                ar.data.set_item("spr", _pack.sprite->_name);
            }
        }
    }

    bool CSprite::OnEdit(Entity ent)
    {
        auto r = ImGui::SpriteInput("Sprite##spr", &_pack);

        return r;
    }


    std::span<const Vec2f> CRegion::GetPath() const
    {
        return _points;
    }

    bool CRegion::HitTest(Vec2f testPoint) const
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

    bool CRegion::OnDeserialize(ArchiveParams2& ar)
    {
        auto  pts = ar.data.get_item("p");
        _points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            _points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    void CRegion::OnSerialize(ArchiveParams2& ar)
    {
        msg::Var pts;
        for (auto p : _points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
    }

    bool CRegion::OnEdit(Entity ent)
    {
        auto  r    = ImGui::PointVector("Points##reg", &_points, {-1, 100});
        return r;
    }

    bool CRegion::OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
    {
        bool   ret  = false;

        ImVec2 snap{1, 1};
        std::swap(snap, canvas.snap_grid);
        for (size_t i = 0; i < _points.size(); ++i)
        {
            auto& p1 = (ImVec2&)_points[i];
            auto& p2 = (ImVec2&)_points[(i + 1) % _points.size()];

            ret |= canvas.DragPoint(p1, &_points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }
        std::swap(snap, canvas.snap_grid);
        return ret;
    }



    bool CCamera::OnDeserialize(ArchiveParams2& ar)
    {
        _position.x   = ar.data["x"].get(0.f);
        _position.y   = ar.data["y"].get(0.f);
        _size.x       = ar.data["w"].get(0.f);
        _size.y       = ar.data["h"].get(0.f);
        _moveSpeed    = ar.data["ms"].get(0.f);
        _zoom         = ar.data["z"].get(0.f);
        _zoomSpeed    = ar.data["zs"].get(0.f);
        _smoothFactor = ar.data["sf"].get(0.f);
        return true;
    }

    void CCamera::OnSerialize(ArchiveParams2& ar)
    {
        ar.data.set_item("x", _position.x);
        ar.data.set_item("y", _position.y);
        ar.data.set_item("w", _size.x);
        ar.data.set_item("h", _size.y);
        ar.data.set_item("ms", _moveSpeed);
        ar.data.set_item("z", _zoom);
        ar.data.set_item("zs", _zoomSpeed);
        ar.data.set_item("sf", _smoothFactor);
    }

    bool CCamera::OnEdit(Entity ent)
    {
        auto  r    = ImGui::InputFloat2("Position", &_position.x);
        r |= ImGui::InputFloat2("Size", &_size.x);

        r |= ImGui::DragFloat("Zoom", &_zoom, 0.1f, 0.1f, 10.0f);
        r |= ImGui::InputFloat("Zoom Speed", &_zoomSpeed, 0.01f, 1.0f);
        r |= ImGui::InputFloat("Move Speed", &_moveSpeed, 0.01f, 1.0f);
        r |= ImGui::DragFloat("Smoothing", &_smoothFactor, 0.1f, 0.0f, 20.0f);

        return r;
    }




    bool CPrefab::OnDeserialize(ArchiveParams2& ar)
    {
        auto  uid = ar.data[Sc::Uid].get(0ull);
        return true;
    }

    void CPrefab::OnSerialize(ArchiveParams2& ar)
    {
        ar.data.set_item(Sc::Uid, _data[Sc::Uid]);
    }



    bool CBody::OnDeserialize(ArchiveParams2& ar)
    {
        _speed.x = ar.data["vx"].get(0.f);
        _speed.y = ar.data["vy"].get(0.f);
        return true;
    }

    void CBody::OnSerialize(ArchiveParams2& ar)
    {
        ar.data.set_item("vx", _speed.x);
        ar.data.set_item("vy", _speed.y);
    }

    bool CBody::OnEdit(Entity ent)
    {
        auto r = ImGui::InputFloat2("Speed", &_speed.x);
        return r;
    }




    bool CPath::OnDeserialize(ArchiveParams2& ar)
    {
        auto  pts = ar.data.get_item("p");
        _path.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            _path.emplace_back(pts[i].get(0), pts[i + 1].get(0));
        }
        return true;
    }

    std::span<const Vec2i> CPath::GetPath() const
    {
        return _path;
    }

    void CPath::OnSerialize(ArchiveParams2& ar)
    {
        msg::Var pts;
        for (auto p : _path)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
    }

    bool CPath::OnEdit(Entity ent)
    {
        return false;
    }




    bool CName::OnDeserialize(ArchiveParams2& ar)
    {
        auto id = ar.data.get_item("id");
        _name   = id.str();
        gGameAPI.SetNamedEntity(gGameAPI.context, ar.entity, id.str());
        return true;
    }

    void CName::OnSerialize(ArchiveParams2& ar)
    {
        if (!_name.empty())
            ar.data.set_item("id", _name);
    }

    bool CName::OnEdit(Entity ent)
    {
        static std::string _s_buff;
        auto&              base = ComponentTraits<CBase>::Get(ent);

        if (!base._layer)
            return false;

        _s_buff     = _name;
        auto* scene = static_cast<ObjectSceneLayer*>(base._layer)->GetScene();

        if (ImGui::InputText("Name", &_s_buff))
        {
            gGameAPI.ClearNamededEntity(gGameAPI.context, _name);
            gGameAPI.SetNamedEntity(gGameAPI.context, ent, _s_buff);
        }
        return false;
    }

    void CAttachment::OnSerialize(ArchiveParams2& ar)
    {
    }

    bool CAttachment::OnDeserialize(ArchiveParams2& ar)
    {
        return true;
    }

    bool CAttachment::OnEdit(Entity self)
    {
        return false;
    }

} // namespace fin
