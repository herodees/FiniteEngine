#include "builtin.hpp"
#include "core/editor/imgui_control.hpp"
#include "core/scene_layer_object.hpp"
#include "core/scene.hpp"

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
        RegBuiltin<CSprite2D>(CSprite2D::CID, "Sprite2D", ComponentsFlags_Default);
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

    Regionf CBase::GetBoundingBox() const
    {
        Regionf bbox{_position.x, _position.y, _position.x, _position.y};
        if (CSprite2D* spr = Find<CSprite2D>(_self))
        {
            if (spr->_spr)
            {
                bbox = spr->GetRegion(_position);
            }

            if (CAttachment* att = Find<CAttachment>(_self))
            {
                auto bbox2 = att->GetBoundingBox();
                bbox.x1    = std::min(bbox.x1, _position.x + bbox2.x1);
                bbox.y1    = std::min(bbox.y1, _position.y + bbox2.y1);
                bbox.x2    = std::max(bbox.x2, _position.x + bbox2.x2);
                bbox.y2    = std::max(bbox.y2, _position.y + bbox2.y2);
            }
        }
        else if (CRegion* reg = Find<CRegion>(_self))
        {
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
                bbox = Region<float>(min + _position, max + _position);
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
        if (Contains<CRegion>(_self))
            return Get<CRegion>(_self).HitTest(pos);

        if (GetBoundingBox().contains(pos))
        {
            return true;
        }
        return false;
    }

    void CBase::OnSerialize(ArchiveParams& ar)
    {
        ar.data.set_item("x", _position.x);
        ar.data.set_item("y", _position.y);
        if (_tags)
        {
            ar.data.set_item("tag", _tags);
        }
    }

    bool CBase::OnDeserialize(ArchiveParams& ar)
    {
        _self       = ar.entity;
        _position.x = ar.data["x"].get(0.f);
        _position.y = ar.data["y"].get(0.f);
        _tags       = ar.data["tag"].get(0u);
        return true;
    }

    bool CBase::OnEdit(Entity ent)
    {
        auto r = ImGui::InputFloat2("Position", &_position.x);
        if (_layer)
        {
            r |= static_cast<ObjectSceneLayer*>(_layer)->GetScene()->ImguiTagInput("Tags", _tags);
        }

        if (r)
        {
            UpdateSparseGrid();
        }
        return r;
    }



    
    bool CIsometric::OnDeserialize(ArchiveParams& ar)
    {
        _a.x  = ar.data["ax"].get(0.f);
        _a.y  = ar.data["ay"].get(0.f);
        _b.x  = ar.data["bx"].get(0.f);
        _b.y  = ar.data["by"].get(0.f);
        _y  = ar.data["y"].get(0);
        return true;
    }

    void CIsometric::OnSerialize(ArchiveParams& ar)
    {
        ar.data.set_item("ax", _a.x);
        ar.data.set_item("ay", _a.y);
        ar.data.set_item("bx", _b.x);
        ar.data.set_item("by", _b.y);
        if (_y)
            ar.data.set_item("y", _y);
    }

    bool CIsometric::OnEdit(Entity ent)
    {
        auto  r    = ImGui::InputFloat2("A", &_a.x);
        r |= ImGui::InputFloat2("B", &_b.x);
        r |= ImGui::DragInt("Y", &_y, 1.f, 0, 600);
        return r;
    }

    bool CIsometric::OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
    {
        bool    ret  = false;

        ImVec2& a    = (ImVec2&)_a;
        ImVec2& b    = (ImVec2&)_b;

        if (!canvas.snap_grid.x || !canvas.snap_grid.y)
            canvas.snap_grid = ImVec2(1, 1);

        ret |= canvas.DragPoint(b, &_b, 5);
        ret |= canvas.DragPoint(a, &_a, 5);

        ImGui::GetWindowDrawList()->AddLine(canvas.WorldToScreen(a), canvas.WorldToScreen(b), IM_COL32(255, 255, 255, 255), 2);
        if (_y > 0)
        {
            ImGui::GetWindowDrawList()->AddLine(canvas.WorldToScreen({a.x, a.y + _y}),
                                                canvas.WorldToScreen({b.x, b.y + _y}),
                                                IM_COL32(255, 0, 255, 255),
                                                2);
        }
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(a), 5, IM_COL32(0, 255, 0, 255));
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(b), 5, IM_COL32(255, 0, 0, 255));

        return ret;
    }




    bool CCollider::OnDeserialize(ArchiveParams& ar)
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

    void CCollider::OnSerialize(ArchiveParams& ar)
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

        if (!canvas.snap_grid.x || !canvas.snap_grid.y)
            canvas.snap_grid = ImVec2(1, 1);

        for (size_t i = 0; i < _points.size(); ++i)
        {
            auto& p1 = (ImVec2&)_points[i];
            auto& p2 = (ImVec2&)_points[(i + 1) % _points.size()];

            ret |= canvas.DragPoint(p1, &_points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }

        return ret;
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

    bool CRegion::OnDeserialize(ArchiveParams& ar)
    {
        auto  pts = ar.data.get_item("p");
        _points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            _points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    void CRegion::OnSerialize(ArchiveParams& ar)
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

        if (!canvas.snap_grid.x || !canvas.snap_grid.y)
            canvas.snap_grid = ImVec2(1, 1);

        for (size_t i = 0; i < _points.size(); ++i)
        {
            auto& p1 = (ImVec2&)_points[i];
            auto& p2 = (ImVec2&)_points[(i + 1) % _points.size()];

            ret |= canvas.DragPoint(p1, &_points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }

        return ret;
    }



    bool CCamera::OnDeserialize(ArchiveParams& ar)
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

    void CCamera::OnSerialize(ArchiveParams& ar)
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




    bool CPrefab::OnDeserialize(ArchiveParams& ar)
    {
        auto  uid = ar.data[Sc::Uid].get(0ull);
        return true;
    }

    void CPrefab::OnSerialize(ArchiveParams& ar)
    {
        ar.data.set_item(Sc::Uid, _data[Sc::Uid]);
    }



    bool CBody::OnDeserialize(ArchiveParams& ar)
    {
        _speed.x = ar.data["vx"].get(0.f);
        _speed.y = ar.data["vy"].get(0.f);
        return true;
    }

    void CBody::OnSerialize(ArchiveParams& ar)
    {
        ar.data.set_item("vx", _speed.x);
        ar.data.set_item("vy", _speed.y);
    }

    bool CBody::OnEdit(Entity ent)
    {
        auto r = ImGui::InputFloat2("Speed", &_speed.x);
        return r;
    }




    bool CPath::OnDeserialize(ArchiveParams& ar)
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

    void CPath::OnSerialize(ArchiveParams& ar)
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




    bool CName::OnDeserialize(ArchiveParams& ar)
    {
        auto id = ar.data.get_item("id");
        _name   = id.str();
        gGameAPI.SetNamedEntity(gGameAPI.context, ar.entity, id.str());
        return true;
    }

    void CName::OnSerialize(ArchiveParams& ar)
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

    int CAttachment::Append(Sprite2D::Ptr ref, Vec2f off)
    {
        int n{};
        for (auto& el : _items)
        {
            ++n;
            if (el._sprite)
                continue;
            el._sprite = ref;
            el._offset = off;
            _bbox.x1   = FLT_MIN;
            return n;
        }
        return -1;
    }

    void CAttachment::Remove(int idx)
    {
        if (idx < 0 || idx >= (int)_items.size())
            return;
        _items[idx] = {};
        _bbox.x1    = FLT_MIN;
    }

    void CAttachment::MoveTo(int idx, Vec2f off)
    {
        if (idx < 0 || idx >= (int)_items.size())
            return;
        _items[idx]._offset = off;
        _bbox.x1            = FLT_MIN;
    }

    void CAttachment::Clear()
    {
        for (auto& el : _items)
            el = {};
        _bbox = {};
    }

    Region<float> CAttachment::GetBoundingBox() const
    {
        if (_bbox.x1 == FLT_MIN)
        {
            _bbox.x1 = FLT_MAX;
            _bbox.y1 = FLT_MAX;
            _bbox.x2 = FLT_MIN;
            _bbox.y2 = FLT_MIN;
            for (auto& el : _items)
            {
                if (el._sprite)
                {
                    _bbox.x1 = std::min(_bbox.x1, el._offset.x - el._sprite->GetOrigin().x);
                    _bbox.y1 = std::min(_bbox.y1, el._offset.y - el._sprite->GetOrigin().y); 
                    _bbox.x2 = std::max(_bbox.x2, el._offset.x - el._sprite->GetOrigin().x + el._sprite->GetSize().x);
                    _bbox.y2 = std::max(_bbox.y2, el._offset.y - el._sprite->GetOrigin().y + el._sprite->GetSize().y); 
                }
            }
            if (_bbox.x1 == FLT_MIN)
                _bbox = {};
        }
        return _bbox;
    }

    void CAttachment::OnSerialize(ArchiveParams& ar)
    {
        msg::Var items;
        for (auto& el : _items)
        {
            msg::Var item;
            item.set_item("src", el._sprite ? el._sprite->GetPath() : std::string{});
            item.set_item("x", el._offset.x);
            item.set_item("y", el._offset.y);
            items.push_back(item);
        }

        if (!items.is_undefined())
        {
            ar.data.set_item("items", items);
        }
    }

    bool CAttachment::OnDeserialize(ArchiveParams& ar)
    {
        auto items = ar.data.get_item("items");
        auto sze   = items.size();
        if (sze)
        {
            for (size_t n = 0; n < _items.size(); ++n)
            {
                _items[n]._sprite.reset();
                _items[n]._offset        = Vec2f(0, 0);
                if (sze > n)
                {
                    auto el = items.get_item(n);
                    auto src            = el["src"];
                    if (!src.empty())
                    {
                        _items[n]._sprite = Sprite2D::LoadShared(src.str());
                    }
                    _items[n]._offset.x = el["x"].get(0.f);
                    _items[n]._offset.y = el["y"].get(0.f);
                }
            }
        }
        return true;
    }

    bool CAttachment::OnEdit(Entity self)
    {
        bool ret = false;
        for (auto& el : _items)
        {
            ImGui::PushID(&el);
            ret |= ImGui::SpriteInput("Sprite##att", &el._sprite);
            ret |= ImGui::InputFloat2("Offset", &el._offset.x);

            ImGui::PopID();
            ImGui::Separator();
        }
        if (ret)
            _bbox.x1 = FLT_MIN;
        return ret;
    }



    Regionf CSprite2D::GetRegion(Vec2f pos) const
    {
        Vec2f org{pos.x - _origin.x, pos.y - _origin.y};
        if (_spr)
        {
            org -= _spr->GetOrigin();
            return Regionf(org, org + _spr->GetSize());
        }
        return Regionf(org, org);
    }

    bool CSprite2D::IsAlphaVisible(uint32_t x, uint32_t y) const
    {
        if (_spr)
        {
            return _spr->IsAlphaVisible(x, y);
        }
        return false;
    }

    void CSprite2D::OnSerialize(ArchiveParams& ar)
    {
        if (_spr)
            ar.data.set_item("src", _spr->GetPath());
        ar.data.set_item("x", _origin.x);
        ar.data.set_item("y", _origin.y);
    }

    bool CSprite2D::OnDeserialize(ArchiveParams& ar)
    {
        _spr = Sprite2D::LoadShared(ar.data.get_item("src").str());
        _origin.x = ar.data.get_item("x").get(0.f);
        _origin.y = ar.data.get_item("y").get(0.f);
        return !!_spr;
    }

    bool CSprite2D::OnEdit(Entity self)
    {
        bool ret{};
        ret |= ImGui::SpriteInput("Sprite", &_spr);
        ret |= ImGui::InputFloat2("Offset", &_origin.x);
        return ret;
    }

    bool CSprite2D::OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
    {
        if (!_spr)
            return false;

        bool ret = false;
        if (!canvas.snap_grid.x || !canvas.snap_grid.y)
            canvas.snap_grid = ImVec2(1, 1);

        if (auto* base = Find<CBase>(ent))
        {
            ImVec2  mouse_pos = canvas.ScreenToWorld(ImGui::GetIO().MousePos);
            Regionf reg(base->_position.x - _origin.x,
                        base->_position.y - _origin.y,
                        base->_position.x - _origin.x,
                        base->_position.y - _origin.y);
            reg.x2 += _spr->GetSize().x;
            reg.y2 += _spr->GetSize().y;

            if (reg.contains(mouse_pos))
            {
                ImVec2 org{-_origin.x, -_origin.y};
                canvas.BeginDrag(org, this);
            }
        }

        ImVec2 org{-_origin.x, -_origin.y};
        if (canvas.EndDrag(org, this))
        {
            _origin.x = -org.x;
            _origin.y = -org.y;
            ret       = true;
        }

        return ret;
    }

} // namespace fin
