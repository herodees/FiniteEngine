#include "scene_layer_object.hpp"
#include "editor/imgui_control.hpp"
#include "utils/imguiline.hpp"
#include "ecs/builtin.hpp"
#include "application.hpp"
#include "renderer.hpp"
#include "scene.hpp"

namespace fin
{
    static bool        s_attachment_mode{false};
    static Entity      s_attachment_target{entt::null};
    static ImVec2      s_attachment_offset{0, 0};
    static int32_t     s_attachment_id{-1};
    static Sprite2D::Ptr     s_attachment_sprite{};
    static int32_t     s_max_visibility{1000};
    static msg::Var    s_copy;

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
        auto& base = Get<CBase>(ent);

        _ptr           = ent;
        _depth        = 0;
        _depth_active = false;
        _origin.point1 = base._position;
        _origin.point2 = _origin.point1;

        _bbox = base.GetBoundingBox();

        if (auto* iso = Find<CIsometric>(ent))
        {
            _origin.point1 += iso->_a;
            _origin.point2 += iso->_b;
            _origin.point1.y += iso->_y;
            _origin.point2.y += iso->_y;
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

        auto& fact = GetScene()->GetFactory();

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

        auto& fact  = GetScene()->GetFactory();
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
        if (auto* obj = Find<CBase>(ent))
        {
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
        if (auto* obj = Find<CBase>(ent))
        {
            if (obj->_layer)
            {
                auto* lyr = static_cast<ObjectSceneLayer*>(obj->_layer);
                lyr->_spatial_db.remove_from_bin(obj);
                lyr->_objects.erase(ent);
            }
            _dirty_navmesh = true;
        }
        GetScene()->GetFactory().GetRegister().Destroy(ent);
    }

    Entity ObjectSceneLayer::FindAt(Vec2f position) const
    {
        Entity ret = entt::null;
        auto   cb  = [&ret, position](lq::SpatialDatabase::Proxy* obj, float dist)
        {
            auto ent = static_cast<CBase*>(obj)->_self;
            if (auto* spr = Find<CSprite2D>(ent))
            {
                if (spr->_spr)
                {
                    auto rc = spr->GetRegion(static_cast<CBase*>(obj)->_position);
                    if (spr->_spr->IsAlphaVisible(position.x - rc.x1, position.y - rc.y1))
                    {
                        ret = ent;
                    }
                }
            }
        };
        _spatial_db.map_over_all_objects_in_locality(position.x, position.y, TileSize, cb);
        return entt::null;
    }

    Entity ObjectSceneLayer::FindActiveAt(Vec2f position) const
    {
        for (auto it = _iso.rbegin(); it != _iso.rend(); ++it)
        {
            auto  obj  = (*it)->_ptr;
            auto& bbox = (*it)->_bbox;

            if (bbox.contains(position))
            {
                if (auto* spr = Find<CSprite2D>(obj))
                {
                    if (spr->_spr)
                    {
                        auto& base = Get<CBase>(obj);
                        auto  rc   = spr->GetRegion(base._position);
                        if (spr->_spr->IsAlphaVisible(position.x - rc.x1, position.y - rc.y1))
                        {
                            return obj;
                        }
                    }
                }
                else
                {
                    return obj;
                }
            }
        }
        return entt::null;
    }

    Entity ObjectSceneLayer::FindActiveAttachmentAt(Vec2f position, int32_t& attachment) const
    {
        attachment = -1;
        for (auto it = _iso.rbegin(); it != _iso.rend(); ++it)
        {
            auto  obj  = (*it)->_ptr;
            auto& bbox = (*it)->_bbox;

            if (bbox.contains(position))
            {
                if (auto* spr = Find<CAttachment>(obj))
                {
                    for (auto it = spr->_items.rbegin(); it != spr->_items.rend(); ++it)
                    {
                        auto& item = *it;
                        if (item._sprite)
                        {
                            Vec2f box;
                            box.x = item._offset.x + Get<CBase>(obj)._position.x - item._sprite->GetOrigin().x;
                            box.y = item._offset.y + Get<CBase>(obj)._position.y - item._sprite->GetOrigin().y;
                            if (item._sprite->IsAlphaVisible(position.x - box.x, position.y - box.y))
                            {
                                attachment = int32_t(std::distance(spr->_items.data(), &item));
                                return obj;
                            }
                        }
                    }
                }
            }
        }
        return entt::null;
    }

    std::span<Vec2i> ObjectSceneLayer::FindPath(Vec2i from, Vec2i to) const
    {
        static std::vector<Vec2i> path;
        _navmesh.findPath(from, to, path);
        return path;
    }

    void ObjectSceneLayer::SelectEdit(Entity ent)
    {
        _edit = ent;
    }

    void ObjectSceneLayer::MoveTo(Entity ent, Vec2f pos)
    {
        auto& obj      = Get<CBase>(ent);
        obj._position = pos;
        Update(&obj);
        _dirty_navmesh = true;
    }

    void ObjectSceneLayer::Move(Entity ent, Vec2f pos)
    {
        auto& obj = Get<CBase>(ent);
        obj._position += pos;
        Update(&obj);
        _dirty_navmesh = true;
    }

    void ObjectSceneLayer::Update(void* obj)
    {
        _spatial_db.update_for_new_location(reinterpret_cast<CBase*>(obj));
    }

    void ObjectSceneLayer::Update(float dt)
    {
        if (IsDisabled())
            return;

        GetScene()->GetFactory().OnLayerUpdate(dt, _selected);
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
        _grid_size.x = (size.width + (TileSize - 1)) / TileSize; // Round up division
        _grid_size.y = (size.height + (TileSize - 1)) / TileSize;
        _spatial_db.init({0, 0, (float)_grid_size.x * TileSize, (float)_grid_size.y * TileSize},
                         _grid_size.x,
                         _grid_size.y);

        for (Entity et : _objects)
        {
            if (auto* obj = Find<CBase>(et))
            {
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

        auto cb = [&](lq::SpatialDatabase::Proxy* item)
        {
            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 64);

            auto& obj = _iso_pool[_iso_pool_size++];
            auto ent = static_cast<CBase*>(item)->_self;

            obj.setup(ent);
        };


        // Query active region
        _spatial_db.map_over_all_objects_in_locality({(float)region.x - TileSize,
                                                      (float)region.y - TileSize,
                                                      (float)region.width + 2 * TileSize,
                                                      (float)region.height + 2 * TileSize},
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

        auto& reg = GetScene()->GetFactory().GetRegister();
        auto sprites = View<CSprite2D, CBase>();

        for (auto ent : _iso)
        {
            if (s_max_visibility < 1000)
            {
                if (auto* iso = Find<CIsometric>(ent->_ptr))
                {
                    if (s_max_visibility < iso->_y)
                        continue;
                }
            }

            if (sprites.Contains(ent->_ptr))
            {
                auto& base   = Get<CBase>(ent->_ptr);
                auto& sprite = Get<CSprite2D>(ent->_ptr);

                if (sprite._spr)
                {
                    dc.render_texture(sprite._spr->GetTexture()->get_texture(),
                                      sprite._spr->GetRect(),
                                      sprite.GetRegion(base._position).rect());
                }

                if (Contains<CAttachment>(ent->_ptr))
                {
                    auto& att = Get<CAttachment>(ent->_ptr);
                    for (auto& el : att._items)
                    {
                        if (el._sprite)
                        {
                            Rectf dest;
                            dest.x      = base._position.x + el._offset.x - el._sprite->GetOrigin().x;
                            dest.y      = base._position.y + el._offset.y - el._sprite->GetOrigin().y;
                            dest.width  = el._sprite->GetSize().x;
                            dest.height = el._sprite->GetSize().y;
                            dc.render_texture(el._sprite->GetTexture()->get_texture(), el._sprite->GetRect(), dest);
                        }
                    }
                }

                if (s_attachment_target == ent->_ptr && s_attachment_sprite)
                {
                    Rectf dest;
                    dest.x      = base._position.x + s_attachment_offset.x - s_attachment_sprite->GetOrigin().x;
                    dest.y      = base._position.y + s_attachment_offset.y - s_attachment_sprite->GetOrigin().y;
                    dest.width  = s_attachment_sprite->GetSize().x;
                    dest.height = s_attachment_sprite->GetSize().y;
                    dc.render_texture(s_attachment_sprite->GetTexture()->get_texture(), s_attachment_sprite->GetRect(), dest);
                }
            }
        }
    }

    bool ObjectSceneLayer::ImguiWorkspace(ImGui::CanvasParams& canvas)
    {
        _drop = entt::null;
        if (!IsMouseButtonDown(0))
        {
            s_attachment_id     = -1;
            s_attachment_sprite = {};
            s_attachment_target = entt::null;
            canvas.dragging     = false;
        }

        if (IsHidden())
            return false;

        bool   modified  = false;
        ImVec2 mouse_pos = canvas.ScreenToWorld(ImGui::GetIO().MousePos);

        if (ImGui::IsItemClicked(0))
        {
            if (s_attachment_mode)
            {
                s_attachment_id = -1;
                auto el  = FindActiveAttachmentAt(mouse_pos, s_attachment_id);
                SelectEdit(el);
                if (s_attachment_id != -1 && Contains<CAttachment>(_edit))
                {
                    auto& base = Get<CAttachment>(_edit);
                    ImVec2 pos(base._items[s_attachment_id]._offset.x, base._items[s_attachment_id]._offset.y);
                    canvas.BeginDrag(pos, &base);
                    modified = true;
                }
            }
            else
            {
                auto el = FindActiveAt(mouse_pos);
                SelectEdit(el);

                if (auto* base = Find<CBase>(_edit))
                {
                    ImVec2 pos{base->_position.x, base->_position.y};
                    canvas.BeginDrag(pos, base);
                    modified = true;
                }
            }
        }

        if (ImGui::IsItemClicked(1))
        {
            SelectEdit(entt::null);
        }

        if (s_attachment_mode)
        {
            if (s_attachment_id != -1 && Contains<CAttachment>(_edit))
            {
                auto&  base = Get<CAttachment>(_edit);
                ImVec2 pos(base._items[s_attachment_id]._offset.x, base._items[s_attachment_id]._offset.y);
                if (canvas.EndDrag(pos, &base))
                {
                    base.MoveTo(s_attachment_id, pos);
                    modified = true;
                }
            }
        }
        else
        {
            if (auto* base = Find<CBase>(_edit))
            {
                ImVec2 pos{base->_position.x, base->_position.y};
                if (canvas.EndDrag(pos, base))
                {
                    MoveTo(_edit, pos);
                    modified = true;
                }
            }
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY",
                                                                           ImGuiDragDropFlags_AcceptPeekOnly |
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                s_attachment_mode = false;
                IM_ASSERT(payload->DataSize == sizeof(Entity));
                _drop = *(const Entity*)payload->Data;
                if (auto* obj = Find<CBase>(_drop))
                {
                    obj->_position = {mouse_pos.x, mouse_pos.y};
                    modified       = true;
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
                if (auto* obj = Find<CBase>(_drop))
                {
                    obj->_position = {mouse_pos.x, mouse_pos.y};
                    Insert(_drop);
                    SelectEdit(_drop);
                    ImGui::SetWindowFocus(); 
                    modified = true;
                }

                _drop = entt::null;
            }


            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE2D",
                                                                           ImGuiDragDropFlags_AcceptPeekOnly |
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                s_attachment_mode = true;
                if (auto* payload_n = *(fin::Sprite2D**)payload->Data)
                {
                    s_attachment_sprite = payload_n->shared_from_this();
                    s_attachment_target = FindActiveAt(mouse_pos);
                    if (s_attachment_target != entt::null && Contains<CBase>(s_attachment_target))
                    {
                        auto& base = Get<CBase>(s_attachment_target);
                        s_attachment_offset = {mouse_pos.x - base._position.x, mouse_pos.y - base._position.y};
                    }
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE2D", ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto* payload_n = *(fin::Sprite2D**)payload->Data)
                {
                    if (s_attachment_target != entt::null && Contains<CBase>(s_attachment_target))
                    {
                        auto& base          = Get<CBase>(s_attachment_target);
                        s_attachment_offset = {mouse_pos.x - base._position.x, mouse_pos.y - base._position.y};
                        if (!Contains<CAttachment>(s_attachment_target))
                            Emplace<CAttachment>(s_attachment_target);
                        Get<CAttachment>(s_attachment_target).Append(payload_n->shared_from_this(), s_attachment_offset);
                        s_attachment_sprite = {};
                        s_attachment_target = entt::null;
                    }
                    modified = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        auto* dc = ImGui::GetWindowDrawList();

        if (gSettings.visible_navgrid)
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
            if (!Contains<CBase>(ent))
                continue;

            auto& base = Get<CBase>(ent);

            if (gSettings.visible_isometric && Contains<CIsometric>(ent))
            {
                auto& iso = Get<CIsometric>(ent);
                auto  a   = iso._a + base._position;
                auto  b   = iso._b + base._position;
                auto  aa  = canvas.WorldToScreen({a.x, a.y + iso._y});
                auto  bb  = canvas.WorldToScreen({b.x, b.y + iso._y});
                if (iso._y <= s_max_visibility)
                    dc->AddLine(aa, bb, IM_COL32(255, 255, 255, 255), 2);
                else
                    dc->AddLine(aa, bb, IM_COL32(255, 0, 0, 255), 2);

                if (ent == _edit)
                {
                    dc->AddText(aa, IM_COL32(0, 255, 0, 255), "L");
                    dc->AddText(bb, IM_COL32(255, 0, 0, 255), "R");
                }
            }

            if (gSettings.visible_collision && Contains<CCollider>(ent))
            {
                auto& col = Get<CCollider>(ent);
                if (col._points.size() > 1)
                {
                    for (size_t i = 0; i < col._points.size(); ++i)
                    {
                        auto p1 = col._points[i] + base._position;
                        auto p2 = col._points[(i + 1) % col._points.size()] + base._position;
                        dc->AddLine(canvas.WorldToScreen(ImVec2(p1.x, p1.y)),
                                    canvas.WorldToScreen(ImVec2(p2.x, p2.y)),
                                    IM_COL32(255, 0, 0, 255),
                                    1.0f);
                    }
                }
            }

            if (ent == _edit)
            {
                auto bb = base.GetBoundingBox();
                dc->AddRect(canvas.WorldToScreen({bb.x1, bb.y1}),
                            canvas.WorldToScreen({bb.x2, bb.y2}),
                            IM_COL32(255, 255, 255, 255),
                            0.0f,
                            ImDrawFlags_Closed,
                            1.0f);
            }
        }

        if (gSettings.visible_grid)
        {
            auto from_x = std::clamp(int32_t((_region.x - TileSize) / TileSize), 0, _grid_size.x);
            auto from_y = std::clamp(int32_t((_region.y - TileSize) / TileSize), 0, _grid_size.y);
            auto to_x   = std::clamp(int32_t((_region.x + _region.width + TileSize) / TileSize), 0, _grid_size.x);
            auto to_y   = std::clamp(int32_t((_region.y + _region.height + TileSize) / TileSize), 0, _grid_size.y);

            for (int x = from_x; x <= to_x; ++x)
            {
                dc->AddLine(canvas.WorldToScreen({(float)x * TileSize, (float)from_y * TileSize}),
                            canvas.WorldToScreen({(float)x * TileSize, (float)to_y * TileSize}),
                            IM_COL32(255, 255, 0, 190),
                            1.0f);
            }
            for (int y = from_y; y <= to_y; ++y)
            {
                dc->AddLine(canvas.WorldToScreen({(float)from_x * TileSize, (float)y * TileSize}),
                            canvas.WorldToScreen({(float)to_x * TileSize, (float)y * TileSize}),
                            IM_COL32(255, 255, 0, 190),
                            1.0f);
            }
        }

        return modified;
    }

    bool ObjectSceneLayer::ImguiWorkspaceMenu(ImGui::CanvasParams& canvas)
    {
        BeginDefaultMenu("wsmnu", canvas);
        ImGui::Line()
            .PushStyle(ImStyle_Button, 10, !s_attachment_mode)
            .Tooltip("Object mode")
            .Space()
            .Text(ICON_FA_GHOST)
            .Space()
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, 20, s_attachment_mode)
            .Tooltip("Attachment mode")
            .Space()
            .Text(ICON_FA_PAPERCLIP)
            .Space()
            .PopStyle();

        if (EndDefaultMenu(canvas))
        {
            if (ImGui::Line().HoverId() == 10)
                s_attachment_mode = false;
            if (ImGui::Line().HoverId() == 20)
                s_attachment_mode = true;
        }

        return false;
    }

    void ObjectSceneLayer::ImguiSetup()
    {
        SceneLayer::ImguiSetup();

        if (ImGui::InputInt2("Navmesh grid", &_cell_size.x))
        {
            _dirty_navmesh = true;
        }
    }

    bool ObjectSceneLayer::ImguiUpdate(bool items)
    {
        bool modified = false;
        if (items)
        {
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderInt("##maxvis", &s_max_visibility, 0, 1000);

            ImGui::LineItem(ImGui::GetID(this), {-1, ImGui::GetFrameHeightWithSpacing()})
                .Space()
                .PushStyle(ImStyle_Button, 10, gSettings.list_visible_items)
                .Text(gSettings.list_visible_items ? " " ICON_FA_EYE " " : " " ICON_FA_EYE_SLASH " ")
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
                    modified = true;
                    return true;
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    gSettings.list_visible_items = !gSettings.list_visible_items;
                }
            }

            if (ImGui::BeginChildFrame(ImGui::GetID("obj"), {-1, -1}, 0))
            {
                if (gSettings.list_visible_items)
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
            modified |= GetScene()->GetFactory().ImguiPrefab(GetScene(), _edit);
        }

        if (items)
        {
            auto& reg = GetScene()->GetFactory().GetRegister();

            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            {
                if (reg.Valid(_edit))
                {
                    Remove(_edit);
                    _edit    = entt::null;
                    modified = true;
                }
            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
            {
                if (reg.Valid(_edit))
                    GetScene()->GetFactory().SaveEntity(_edit, s_copy);
            }
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X, false))
            {
                if (reg.Valid(_edit))
                {
                    GetScene()->GetFactory().SaveEntity(_edit, s_copy);
                    Remove(_edit);
                    _edit = entt::null;
                }
            }
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && s_copy.is_object())
            {
                GetScene()->GetFactory().ClearOldEntities();
                Entity ent{};
                GetScene()->GetFactory().LoadEntity(ent, s_copy);
                Insert(ent);
                MoveTo(ent, _region.center());
                SelectEdit(ent);
            }
        }
        return modified;
    }

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

    ObjectLayer* ObjectSceneLayer::Objects()
    {
        return this;
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
            if (Contains<CBase>(et) && Contains<CCollider>(et))
            {
                auto& obj = Get<CBase>(et);
                auto& col = Get<CCollider>(et);
                points.clear();
                std::for_each(col._points.begin(),
                              col._points.end(),
                              [&points, obj](const Vec2f& pt) { points.push_back(pt + obj._position); });

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
