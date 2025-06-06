#include "scene_layer_object.hpp"
#include "editor/imgui_control.hpp"
#include "utils/imguiline.hpp"
#include "ecs/base.hpp"
#include "application.hpp"
#include "renderer.hpp"
#include "scene.hpp"

namespace fin
{
    int32_t ObjectSceneLayer::IsoObject::depth_get()
    {
        if (_depth_active)
            return 0;

        if (!_depth)
        {
            _depth_active = true;
            if (_back.empty())
                _depth = 1;
            else
            {
                for (auto el : _back)
                    _depth = std::max(el->depth_get(), _depth);
                _depth += 1;
            }
            _depth_active = false;
        }
        return _depth;
    }

    void ObjectSceneLayer::IsoObject::setup(Entity ent)
    {
        auto* base = ecs::Base::Get(ent);

        _ptr           = ent;
        _depth        = 0;
        _depth_active = false;
        _origin.point1 = base->_position;
        _origin.point2 = _origin.point1;

        _bbox = base->GetBoundingBox();

        if (ecs::Isometric::Contains(ent))
        {
            auto* iso = ecs::Isometric::Get(ent);
            _origin.point1 += iso->_a;
            _origin.point2 += iso->_b;
        }

        _back.clear();
    }


    ObjectSceneLayer::ObjectSceneLayer() : SceneLayer(LayerType::Object), _navmesh(0, 0, _cell_size.x, _cell_size.y)
    {
        GetName() = "ObjectLayer";
        GetIcon() = ICON_FA_MAP_PIN;
        _color = 0xffa0a0ff;
    };

    ObjectSceneLayer ::~ObjectSceneLayer()
    {
        Clear();
    }

    void ObjectSceneLayer::Serialize(msg::Var& ar)
    {
        SceneLayer::Serialize(ar);
        msg::Var items;
        items.make_array(_objects.size());

        auto& fact = parent()->GetFactory();

        for (auto ent : _objects)
        {
            msg::Var obj;
            fact.SaveEntity(ent, obj);
            items.push_back(obj);
        }

        ar.set_item("items", items);
        ar.set_item("cw", _cell_size.x);
        ar.set_item("ch", _cell_size.y);
    }

    void ObjectSceneLayer::Deserialize(msg::Var& ar)
    {
        SceneLayer::Deserialize(ar);

        auto& fact  = parent()->GetFactory();
        _cell_size.x = ar.get_item("cw").get(16);
        _cell_size.y = ar.get_item("ch").get(8);
        auto items = ar.get_item("items");
        for (auto& obj : items.elements())
        {
            Entity ent{entt::null};
            fact.LoadEntity(ent, obj);
            if (ent != entt::null)
                Insert(ent);
        }
        UpdateNavmesh();
    }

    void ObjectSceneLayer::Insert(Entity ent)
    {
        if (ecs::Base::Contains(ent))
        {
            auto* obj = ecs::Base::Get(ent);
            if (obj->_layer == this)
                return;

            if (obj->_layer)
                obj->_layer->Remove(ent);

            obj->_layer = this;
            _spatial_db.update_for_new_location(obj);
            _dirty_navmesh = true;
        }
        _objects.emplace(ent);
    }

    void ObjectSceneLayer::Remove(Entity ent)
    {
        if (ecs::Base::Contains(ent))
        {
            auto* obj = ecs::Base::Get(ent);
            if (obj->_layer)
            {
                obj->_layer->_spatial_db.remove_from_bin(obj);
                obj->_layer->_objects.erase(ent);
            }
            _dirty_navmesh = true;
        }
        parent()->GetFactory().GetRegistry().destroy(ent);
    }

    Entity ObjectSceneLayer::FindAt(Vec2f position) const
    {
        Entity ret = entt::null;
        auto   cb  = [&ret, position](lq::SpatialDatabase::Proxy* obj, float dist)
            {
                auto ent = static_cast<ecs::Base*>(obj)->_self;
                if (ecs::Sprite::Contains(ent))
                {
                    auto* spr = ecs::Sprite::Get(ent);
                    if (spr->pack.sprite)
                    {
                        Vec2f bbox;
                        bbox.x = static_cast<ecs::Base*>(obj)->_position.x - spr->pack.sprite->_origina.x;
                        bbox.y = static_cast<ecs::Base*>(obj)->_position.y - spr->pack.sprite->_origina.y;
                        if (spr->pack.is_alpha_visible(position.x - bbox.x, position.y - bbox.y))
                        {
                            ret = ent;
                        }
                    }
                }
            };

        _spatial_db.map_over_all_objects_in_locality(position.x, position.y, tile_size, cb);
        return entt::null;
    }

    Entity ObjectSceneLayer::FindActiveAt(Vec2f position) const
    {
        for (auto it = _iso.rbegin(); it != _iso.rend(); ++it)
        {
            auto obj  = (*it)->_ptr;
            auto& bbox = (*it)->_bbox;

            if (bbox.contains(position))
            {
                if (ecs::Sprite::Contains(obj))
                {
                    auto* spr = ecs::Sprite::Get(obj);
                    if (spr->pack.sprite)
                    {
                        if (spr->pack.is_alpha_visible(position.x - bbox.x1, position.y - bbox.y1))
                        {
                            return obj;
                        }
                    }
                    else
                        return obj;
                }
                else
                {
                    return obj;
                }
            }
        }
        return entt::null;
    }

    void ObjectSceneLayer::SelectEdit(Entity ent)
    {
        _edit = ent;
    }

    void ObjectSceneLayer::MoveTo(Entity ent, Vec2f pos)
    {
        auto* obj      = ecs::Base::Get(ent);
        obj->_position = pos;
        Update(obj);
        _dirty_navmesh = true;
    }

    void ObjectSceneLayer::Move(Entity ent, Vec2f pos)
    {
        auto* obj      = ecs::Base::Get(ent);
        obj->_position += pos;
        Update(obj);
        _dirty_navmesh = true;
    }

    void ObjectSceneLayer::Update(void* obj)
    {
        _spatial_db.update_for_new_location(reinterpret_cast<ecs::Base*>(obj));
    }

    void ObjectSceneLayer::Update(float dt)
    {
        if (IsDisabled())
            return;

        auto& objects = GetObjects(true);
        auto& fact = Scene().GetFactory();
        auto& map = fact.GetBehaviors();

        for (auto& [k, v] : map)
        {
            v.update(objects, dt);
        }
    }        

    void ObjectSceneLayer::Clear()
    {
        _iso_pool.clear();
        _iso.clear();
        _grid_size     = {};
        _iso_pool_size = {};
        _objects.clear();
    }

    void ObjectSceneLayer::Resize(Vec2f size)
    {
        _size        = size;
        _grid_size.x = (size.width + (tile_size - 1)) / tile_size; // Round up division
        _grid_size.y = (size.height + (tile_size - 1)) / tile_size;
        _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                         _grid_size.x,
                         _grid_size.y);

        for (Entity et : _objects)
        {
            if (ecs::Base::Contains(et))
            {
                auto* obj = ecs::Base::Get(et);
                obj->_bin   = nullptr;
                obj->_prev  = nullptr;
                obj->_next  = nullptr;
                _spatial_db.update_for_new_location(obj);
            }
        }
        _dirty_navmesh = true;
    }

    void ObjectSceneLayer::Activate(const Rectf& region)
    {
        SceneLayer::Activate(region);

        _selected.clear();
        _iso_pool_size = 0;
        auto& reg = parent()->GetFactory().GetRegistry();

        auto cb = [&](lq::SpatialDatabase::Proxy* item)
        {
            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 64);

            auto& obj = _iso_pool[_iso_pool_size++];
            auto ent = static_cast<ecs::Base*>(item)->_self;

            obj.setup(ent);
        };


        // Query active region
        _spatial_db.map_over_all_objects_in_locality({(float)region.x - tile_size,
                                                      (float)region.y - tile_size,
                                                      (float)region.width + 2 * tile_size,
                                                      (float)region.height + 2 * tile_size},
                                                     cb);

        if (_iso_pool_size >= _iso_pool.size())
            _iso_pool.resize(_iso_pool_size + 32);

        // Reset iso depth and sort data
        _iso.resize(_iso_pool_size);
        for (size_t n = 0; n < _iso_pool_size; ++n)
        {
            _iso[n] = &_iso_pool[n];
            _selected.emplace(_iso_pool[n]._ptr);
        }

        // Add edit object to render queue
        if (_drop != entt::null)
        {
            _iso_pool[_iso_pool_size].setup(_drop);
            _iso.emplace_back();
            _iso[_iso_pool_size] = &_iso_pool[_iso_pool_size];
            _selected.emplace(_iso_pool[_iso_pool_size]._ptr);
        }


        // Topology sort
        if (_iso.size() > 1)
        {
            // Determine depth relationships
            for (size_t i = 0; i < _iso.size(); ++i)
            {
                auto* a = _iso[i];
                for (size_t j = i + 1; j < _iso.size(); ++j)
                {
                    auto* b = _iso[j];
                    // Ignore non overlaped rectangles
                    if (a->_bbox.intersects(b->_bbox))
                    {
                        // Check if x2 is above iso line
                        if (b->_origin.compare(a->_origin) >= 0)
                        {
                            a->_back.push_back(b);
                        }
                        else
                        {
                            b->_back.push_back(a);
                        }
                    }
                }
            }

            // Recursive function to calculate depth
            for (auto* obj : _iso)
                obj->depth_get();

            // Sort objects by depth
            std::sort(_iso.begin(),
                      _iso.end(),
                      [](const IsoObject* a, const IsoObject* b) { return a->_depth < b->_depth; });
        }
    }

    void ObjectSceneLayer::Render(Renderer& dc)
    {
        if (IsHidden())
            return;

        dc.set_color(WHITE);

        auto& reg = parent()->GetFactory().GetRegistry();
        auto sprites = reg.view<ecs::Sprite, ecs::Base>();

        for (auto ent : _iso)
        {
            if (sprites.contains(ent->_ptr))
            {
                auto* base   = ecs::Base::Get(ent->_ptr);
                auto* sprite = ecs::Sprite::Get(ent->_ptr);

                if (sprite->pack.sprite)
                {
                    Rectf dest;
                    dest.x      = base->_position.x - sprite->pack.sprite->_origina.x;
                    dest.y      = base->_position.y - sprite->pack.sprite->_origina.y;
                    dest.width  = sprite->pack.sprite->_source.width;
                    dest.height = sprite->pack.sprite->_source.height;

                    dc.render_texture(sprite->pack.sprite->_texture, sprite->pack.sprite->_source, dest);
                }
            }
        }
    }

    void ObjectSceneLayer::ImguiWorkspace(ImGui::CanvasParams& canvas)
    {
        _drop = entt::null;

        if (IsHidden())
            return;

        ImVec2 mouse_pos = canvas.ScreenToWorld(ImGui::GetIO().MousePos);

        if (ImGui::IsItemClicked(0))
        {
            auto el = FindActiveAt(mouse_pos);
            SelectEdit(el);

            if (ecs::Base::Contains(_edit))
            {
                auto*  base = ecs::Base::Get(_edit);
                ImVec2 pos{base->_position.x, base->_position.y};
                canvas.BeginDrag(pos, base);
            }
        }

        if (ImGui::IsItemClicked(1))
        {
            SelectEdit(entt::null);
        }

        if (ecs::Base::Contains(_edit))
        {
            auto*   base = ecs::Base::Get(_edit);
            ImVec2 pos{base->_position.x, base->_position.y};
            if (canvas.EndDrag(pos, base))
            {
                MoveTo(_edit, pos);
            }
        }
   
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY",
                                                                           ImGuiDragDropFlags_AcceptPeekOnly |
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                IM_ASSERT(payload->DataSize == sizeof(Entity));
                _drop = *(const Entity*)payload->Data;
                if (ecs::Base::Contains(_drop))
                {
                    auto* obj      = ecs::Base::Get(_drop);
                    obj->_position = {mouse_pos.x, mouse_pos.y};
                }
                else
                {
                    _drop = entt::null;
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                IM_ASSERT(payload->DataSize == sizeof(Entity));
                _drop = *(const Entity*)payload->Data;
                if (ecs::Base::Contains(_drop))
                {
                    auto* obj      = ecs::Base::Get(_drop);
                    obj->_position = {mouse_pos.x, mouse_pos.y};
                    Insert(_drop);
                }

                _drop = entt::null;
            }

            ImGui::EndDragDropTarget();
        }

        auto* dc = ImGui::GetWindowDrawList();

        if (g_settings.visible_navgrid)
        {
            UpdateNavmesh();
            auto& txt = _navmesh.getDebugTexture();
            ImGui::DrawTexture(canvas,
                               (ImTextureID)&txt,
                               {0, 0},
                               {(float)_navmesh.worldSize().x, (float)_navmesh.worldSize().y},
                               {0, 0},
                               {1.f, 1.f},
                               IM_COL32(255, 255, 255, 180));
        }

        for (auto ent : _selected)
        {
            if (!ecs::Base::Contains(ent))
                continue;

            auto* base = ecs::Base::Get(ent);

            if (g_settings.visible_isometric && ecs::Isometric::Contains(ent))
            {
                auto* iso = ecs::Isometric::Get(ent);
                auto  a   = iso->_a + base->_position;
                auto  b   = iso->_b + base->_position;
                auto  aa  = canvas.WorldToScreen({a.x, a.y});
                auto  bb  = canvas.WorldToScreen({b.x, b.y});
                dc->AddLine(aa, bb, IM_COL32(255, 255, 255, 255), 2);

                if (ent == _edit)
                {
                    dc->AddText(aa, IM_COL32(0, 255, 0, 255), "L");
                    dc->AddText(bb, IM_COL32(255, 0, 0, 255), "R");
                }
            }

            if (g_settings.visible_collision && ecs::Collider::Contains(ent))
            {
                auto* col = ecs::Collider::Get(ent);
            }

            if (ent == _edit)
            {
                auto bb = base->GetBoundingBox();
                dc->AddRect(canvas.WorldToScreen({bb.x1, bb.y1}),
                            canvas.WorldToScreen({bb.x2, bb.y2}),
                            IM_COL32(255, 255, 255, 255),
                            0.0f,
                            ImDrawFlags_Closed,
                            1.0f);
            }
        }

        if (g_settings.visible_grid)
        {

        }
    }

    void ObjectSceneLayer::ImguiWorkspaceMenu()
    {
        BeginDefaultMenu("wsmnu");
        if (EndDefaultMenu())
        {
        }
    }

    void ObjectSceneLayer::ImguiSetup()
    {
        SceneLayer::ImguiSetup();

        if (ImGui::InputInt2("Navmesh grid", &_cell_size.x))
        {
            _dirty_navmesh = true;
        }
    }

    void ObjectSceneLayer::ImguiUpdate(bool items)
    {
        if (items)
        {
            ImGui::LineItem(ImGui::GetID(this), {-1, ImGui::GetFrameHeightWithSpacing()})
                .Space()
                .PushStyle(ImStyle_Button, 10, g_settings.list_visible_items)
                .Text(g_settings.list_visible_items ? " " ICON_FA_EYE " " : " " ICON_FA_EYE_SLASH " ")
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 1, false)
                .Text(" " ICON_FA_BAN " ")
                .PopStyle()
                .End();


            if (ImGui::Line().Return())
            {
                if (_edit != entt::null && ImGui::Line().HoverId() == 1)
                {
                    Remove(_edit);
                    _edit = entt::null;
                    return;
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    g_settings.list_visible_items = !g_settings.list_visible_items;
                }
            }

            if (ImGui::BeginChildFrame(ImGui::GetID("obj"), {-1, -1}, 0))
            {
                if (g_settings.list_visible_items)
                {
                    for (auto& el : _iso)
                    {
                        ImGui::PushID(static_cast<uint32_t>(el->_ptr));

                        const char* name = ImGui::FormatStr("Object %p", el->_ptr);

                        if (ImGui::Selectable(name, el->_ptr == _edit))
                        {
                            SelectEdit(el->_ptr);
                        }
                        ImGui::PopID();
                    }
                }
                else
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(_objects.size());
                    while (clipper.Step())
                    {
                        for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                        {
                            ImGui::PushID(n);
                            auto el   = _objects[n];

                            const char* name = ImGui::FormatStr("Object %p", el);

                            if (ImGui::Selectable(name, el == _edit))
                            {
                                SelectEdit(el);
                            }
                            ImGui::PopID();
                        }
                    }
                }
            }

            ImGui::EndChildFrame();

        }
        else if (_edit != entt::null)
        {
            parent()->GetFactory().ImguiPrefab(parent(), _edit);
        }
    }
    /*
    Entity ObjectSceneLayer::GetActive(size_t n) const
    {
        return _iso[n]->_ptr;
    }

    size_t ObjectSceneLayer::GetActiveCount() const
    {
        return _iso.size();
    }**/

    Navmesh& ObjectSceneLayer::GetNavmesh()
    {
        return _navmesh;
    }

    const Navmesh& ObjectSceneLayer::GetNavmesh() const
    {
        return _navmesh;
    }

    const SparseSet& ObjectSceneLayer::GetObjects(bool active_only) const
    {
        return active_only ? _selected : _objects;
    }

    void ObjectSceneLayer::UpdateNavmesh()
    {
        if (!_dirty_navmesh)
            return;

        _dirty_navmesh = false;
        _navmesh.resize(_size.x, _size.y, _cell_size.x, _cell_size.y);
        _navmesh.resetTerrain();
        std::vector<Vec2f> points;
        for (Entity et : _objects)
        {
            if (ecs::Base::Contains(et) && ecs::Collider::Contains(et))
            {
                auto* obj = ecs::Base::Get(et);
                auto* col = ecs::Collider::Get(et);
                points.clear();
                std::for_each(col->_points.begin(),
                              col->_points.end(),
                              [&points, obj](const Vec2f& pt) { points.push_back(pt + obj->_position); });

                _navmesh.applyTerrain(points, TERRAIN_BLOCKED, true);
            }
        }
    }

    bool ObjectSceneLayer::FindPath(Vec2i from, Vec2i to, std::vector<Vec2i>& path) const
    {
        return _navmesh.findPath(from, to, path);
    }

    SceneLayer* SceneLayer::CreateObject()
    {
        return new ObjectSceneLayer;
    }

} // namespace fin
