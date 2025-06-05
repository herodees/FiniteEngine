#include "base.hpp"
#include <core/editor/imgui_control.hpp>
#include <core/scene_layer.hpp>
#include <core/scene_layer_object.hpp>
#include <core/scene.hpp>

namespace fin::ecs
{
    void RegisterBaseComponents(ComponentFactory& fact)
    {
        fact.RegisterComponent<Base>(ComponentFlags_NoWorkspaceEditor);
        fact.RegisterComponent<Name>(ComponentFlags_NoWorkspaceEditor | ComponentFlags_NoPrefab);
        fact.RegisterComponent<Isometric>();
        fact.RegisterComponent<Collider>();
        fact.RegisterComponent<Sprite>(ComponentFlags_NoWorkspaceEditor);
        fact.RegisterComponent<Region>();
        fact.RegisterComponent<Path>(ComponentFlags_NoWorkspaceEditor);
        fact.RegisterComponent<Camera>(ComponentFlags_NoWorkspaceEditor);
        fact.RegisterComponent<Body>(ComponentFlags_NoWorkspaceEditor); 
        fact.RegisterComponent<Prefab>(ComponentFlags_Private); // Prefab is not editable in the editor
        fact.RegisterComponent<Script>(
            ComponentFlags_NoWorkspaceEditor | ComponentFlags_NoEditor); // Prefab is not editable in the editor
    }

    fin::Region<float> Base::GetBoundingBox() const
    {
        fin::Region<float> bbox{_position.x, _position.y, _position.x, _position.y};
        if (Sprite::Contains(_self))
        {
            auto* spr = Sprite::Get(_self);
            if (spr->pack.sprite)
            {
                bbox.x1 -= spr->pack.sprite->_origina.x;
                bbox.y1 -= spr->pack.sprite->_origina.y;
                bbox.x2 = bbox.x1 + spr->pack.sprite->_source.width;
                bbox.y2 = bbox.y1 + spr->pack.sprite->_source.height;
            }
        }
        else if (Region::Contains(_self))
        {
            auto* reg = Region::Get(_self);
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

    bool Base::HitTest(Vec2f pos) const
    {
        if (Region::Contains(_self))
            return Region::Get(_self)->HitTest(pos);

        if (GetBoundingBox().contains(pos))
        {
            return true;
        }
        return false;
    }

    bool Base::Load(ArchiveParams& ar)
    {
        auto* base = Base::Get(ar.entity);
        base->_self       = ar.entity;
        base->_position.x = ar.data["x"].get(0.f);
        base->_position.y = ar.data["y"].get(0.f);

        return true;
    }

    bool Base::Save(ArchiveParams& ar)
    {
        auto* base = Base::Get(ar.entity);
        ar.data.set_item("x", base->_position.x);
        ar.data.set_item("y", base->_position.y);

        return true;
    }

    bool Base::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto r = ImGui::InputFloat2("Position", &base._position.x);
        if (r)
        {
            base.UpdateSparseGrid();
        }
        return r;
    }

    void Base::UpdateSparseGrid()
    {
        if (_layer)
            _layer->Update(this);
    }

    bool Isometric::Load(ArchiveParams& ar)
    {
        auto& iso  = *Isometric::Get(ar.entity);
        iso._a.x   = ar.data["ax"].get(0.f);
        iso._a.y   = ar.data["ay"].get(0.f);
        iso._b.x   = ar.data["bx"].get(0.f);
        iso._b.y   = ar.data["by"].get(0.f);
        return true;
    }

    bool Isometric::Save(ArchiveParams& ar)
    {
        auto& iso = *Isometric::Get(ar.entity);
        ar.data.set_item("ax", iso._a.x);
        ar.data.set_item("ay", iso._a.y);
        ar.data.set_item("bx", iso._b.x);
        ar.data.set_item("by", iso._b.y);
        return true;
    }

    bool Isometric::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::InputFloat2("A", &base._a.x);
        r |= ImGui::InputFloat2("B", &base._b.x);
        return r;
    }

    bool Isometric::ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self)
    {
        bool ret = false;
        auto& base = GetStorage().get(self);
        ImVec2& a    = (ImVec2&)base._a;
        ImVec2& b    = (ImVec2&)base._b;
        ImVec2  snap{1, 1};
        std::swap(snap, canvas.snap_grid);

        ret |= canvas.DragPoint(b, &base._b, 5);
        ret |= canvas.DragPoint(a, &base._a, 5);

        ImGui::GetWindowDrawList()->AddLine(canvas.WorldToScreen(a), canvas.WorldToScreen(b), IM_COL32(255, 255, 255, 255), 2);
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(a), 5, IM_COL32(0, 255, 0, 255));
        ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(b), 5, IM_COL32(255, 0, 0, 255));

        std::swap(snap, canvas.snap_grid);
        return ret;
    }

    bool Collider::Load(ArchiveParams& ar)
    {
        auto& col = *Collider::Get(ar.entity);
        auto pts = ar.data.get_item("p");
        col._points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col._points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Collider::Save(ArchiveParams& ar)
    {
        auto&    col = *Collider::Get(ar.entity);
        msg::Var pts;
        for (auto p : col._points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
        return true;
    }

    bool Collider::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::PointVector("Points##cldr", &base._points, {-1, 100});
        return r;
    }

    bool Collider::ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self) 
    {
        bool   ret  = false;
        auto&  base = GetStorage().get(self);
        ImVec2 snap{1, 1};
        std::swap(snap, canvas.snap_grid);
        for (size_t i = 0; i < base._points.size(); ++i)
        {
            auto& p1 = (ImVec2&)base._points[i];
            auto& p2 = (ImVec2&)base._points[(i + 1) % base._points.size()];

            ret |= canvas.DragPoint(p1, &base._points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }
        std::swap(snap, canvas.snap_grid);
        return ret;
    }

    bool Sprite::Load(ArchiveParams& ar)
    {
        auto& spr = *Sprite::Get(ar.entity);
        auto  src = ar.data["src"];
        auto  sp = ar.data["spr"];
        spr.pack = Atlas::load_shared(src.str(), sp.str());
        return true;
    }

    bool Sprite::Save(ArchiveParams& ar)
    {
        auto& spr = *Sprite::Get(ar.entity);
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

    bool Sprite::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::SpriteInput("Sprite##spr", &base.pack);

        return r;
    }

    bool Region::HitTest(Vec2f testPoint) const
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

    bool Region::Load(ArchiveParams& ar)
    {
        auto& col = *Region::Get(ar.entity);
        auto  pts = ar.data.get_item("p");
        col._points.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col._points.emplace_back(pts[i].get(0.f), pts[i + 1].get(0.f));
        }
        return true;
    }

    bool Region::Save(ArchiveParams& ar)
    {
        auto&    col = *Region::Get(ar.entity);
        msg::Var pts;
        for (auto p : col._points)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
        return true;
    }

    bool Region::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::PointVector("Points##reg", &base._points, {-1, 100});
        return r;
    }

    bool Region::ImguiWorkspace(ImGui::CanvasParams& canvas, Entity self)
    {
        bool   ret  = false;
        auto&  base = GetStorage().get(self);
        ImVec2 snap{1, 1};
        std::swap(snap, canvas.snap_grid);
        for (size_t i = 0; i < base._points.size(); ++i)
        {
            auto& p1 = (ImVec2&)base._points[i];
            auto& p2 = (ImVec2&)base._points[(i + 1) % base._points.size()];

            ret |= canvas.DragPoint(p1, &base._points[i], 5);

            ImGui::GetWindowDrawList()
                ->AddLine(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), IM_COL32(255, 255, 255, 255), 2);
            ImGui::GetWindowDrawList()->AddCircle(canvas.WorldToScreen(p1), 5, IM_COL32(255, 255, 255, 255));
        }
        std::swap(snap, canvas.snap_grid);
        return ret;
    }

    bool Camera::Load(ArchiveParams& ar)
    {
        auto& cam         = *Camera::Get(ar.entity);
        cam._position.x   = ar.data["x"].get(0.f);
        cam._position.y   = ar.data["y"].get(0.f);
        cam._size.x       = ar.data["w"].get(0.f);
        cam._size.y       = ar.data["h"].get(0.f);
        cam._moveSpeed    = ar.data["ms"].get(0.f);
        cam._zoom         = ar.data["z"].get(0.f);
        cam._zoomSpeed    = ar.data["zs"].get(0.f);
        cam._smoothFactor = ar.data["sf"].get(0.f);
        return true;
    }

    bool Camera::Save(ArchiveParams& ar)
    {
        auto& cam = *Camera::Get(ar.entity);
        ar.data.set_item("x", cam._position.x);
        ar.data.set_item("y", cam._position.y);
        ar.data.set_item("w", cam._size.x);
        ar.data.set_item("h", cam._size.y);
        ar.data.set_item("ms", cam._moveSpeed);
        ar.data.set_item("z", cam._zoom);
        ar.data.set_item("zs", cam._zoomSpeed);
        ar.data.set_item("sf", cam._smoothFactor);
        return true;
    }

    bool Camera::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::InputFloat2("Position", &base._position.x);
        r |= ImGui::InputFloat2("Size", &base._size.x);

        r |= ImGui::DragFloat("Zoom", &base._zoom, 0.1f, 0.1f, 10.0f);
        r |= ImGui::InputFloat("Zoom Speed", &base._zoomSpeed, 0.01f, 1.0f);
        r |= ImGui::InputFloat("Move Speed", &base._moveSpeed, 0.01f, 1.0f);
        r |= ImGui::DragFloat("Smoothing", &base._smoothFactor, 0.1f, 0.0f, 20.0f);

        return r;
    }

    bool Prefab::Load(ArchiveParams& ar)
    {
        auto& pf  = *Prefab::Get(ar.entity);
        auto uid = ar.data[Sc::Uid].get(0ull);
        return true;
    }

    bool Prefab::Save(ArchiveParams& ar)
    {
        auto& pf = *Prefab::Get(ar.entity);
        ar.data.set_item(Sc::Uid, pf._data[Sc::Uid]);
        return true;
    }

    bool Body::Load(ArchiveParams& ar)
    {
        auto& dby           = *Body::Get(ar.entity);
        dby._speed.x        = ar.data["vx"].get(0.f);
        dby._speed.y        = ar.data["vy"].get(0.f);
        return true;
    }

    bool Body::Save(ArchiveParams& ar)
    {
        auto& dby = *Body::Get(ar.entity);
        ar.data.set_item("vx", dby._speed.x);
        ar.data.set_item("vy", dby._speed.y);
        return true;
    }

    bool Body::ImguiProps(Entity self)
    {
        auto& base = GetStorage().get(self);
        auto  r    = ImGui::InputFloat2("Speed", &base._speed.x);
        return r;
    }

    bool Path::Load(ArchiveParams& ar)
    {
        auto& col = *Path::Get(ar.entity);
        auto  pts = ar.data.get_item("p");
        col._path.clear();
        for (size_t i = 0; i < pts.size(); i += 2)
        {
            col._path.emplace_back(pts[i].get(0), pts[i + 1].get(0));
        }
        return true;
    }

    bool Path::Save(ArchiveParams& ar)
    {
        auto&    col = *Path::Get(ar.entity);
        msg::Var pts;
        for (auto p : col._path)
        {
            pts.push_back(p.x);
            pts.push_back(p.y);
        }
        ar.data.set_item("p", pts);
        return true;
    }

    bool Path::ImguiProps(Entity self)
    {
        return false;
    }

    Script::~Script()
    {
        auto* script = _script;
        while (script)
        {
            auto* next = script->next;
            delete script;
            script = next;
        }
    }

    IBehaviorScript* Script::AddScript(std::string_view name) const
    {
        auto* script = _script;
        while (script)
        {
            if (name == script->info->name)
            {
                return script;
            }
            script = script->next;
        }
        return nullptr;
    }

    void Script::AddScript(IBehaviorScript* scr)
    {
        scr->next = _script;
        _script = scr;
    }

    void Script::RemoveScript(IBehaviorScript* scr)
    {
        if (_script == scr)
        {
            _script = scr->next;
            return;
        }
        auto* script = _script;
        while (script && script->next)
        {
            if (script->next == scr)
            {
                script->next = scr->next;
                return;
            }
            script = script->next;
        }
    }

void Script::MoveScript(IBehaviorScript* scr, int dir)
    {
        if (!scr || !_script || dir == 0)
            return;

        // Moving up (dir < 0)
        if (dir < 0)
        {
            // If it's already the first element, can't move up
            if (_script == scr)
                return;

            // Find the previous node
            IBehaviorScript* prev    = nullptr;
            IBehaviorScript* current = _script;
            while (current && current->next != scr)
            {
                prev    = current;
                current = current->next;
            }

            // If we found the node to move
            if (current && current->next == scr)
            {
                if (prev)
                {
                    // Normal case - node is in middle of list
                    IBehaviorScript* scrNext = scr->next;
                    prev->next               = scr;
                    scr->next                = current;
                    current->next            = scrNext;
                }
                else
                {
                    // Special case - moving second node to first
                    IBehaviorScript* scrNext = scr->next;
                    _script                  = scr;
                    scr->next                = current;
                    current->next            = scrNext;
                }
            }
        }
        // Moving down (dir > 0)
        else
        {
            // Find the node and its previous
            IBehaviorScript* prev    = nullptr;
            IBehaviorScript* current = _script;
            while (current && current != scr)
            {
                prev    = current;
                current = current->next;
            }

            // If we found the node and it has a next node to swap with
            if (current == scr && scr->next)
            {
                IBehaviorScript* scrNext = scr->next;
                scr->next                = scrNext->next;
                scrNext->next            = scr;

                if (prev)
                {
                    prev->next = scrNext;
                }
                else
                {
                    _script = scrNext;
                }
            }
        }
    }

    bool Script::Load(ArchiveParams& ar)
    {
        return false;
    }

    bool Script::Save(ArchiveParams& ar)
    {
        return false;
    }

    bool Name::Load(ArchiveParams& ar)
    {
        auto id = ar.data.get_item("id");
        ar.NameEntity(ar.entity, id.str());
        return true;
    }

    bool Name::Save(ArchiveParams& ar)
    {
        auto& nme = *Name::Get(ar.entity);
        if (!nme._name.empty())
            ar.data.set_item("id", nme._name);
        return true;
    }

    bool Name::ImguiProps(Entity self)
    {
        static std::string _s_buff;
        auto&              nme  = *Name::Get(self);
        auto&              base = *Base::Get(self);

        if (!base._layer)
            return false;

        _s_buff = nme._name;
        auto* scene = base._layer->parent();

        if (ImGui::InputText("Name", &_s_buff))
        {
            return scene->GetFactory().SetEntityName(self, _s_buff);
        }
        return false;
    }

} // namespace fin::ecs
