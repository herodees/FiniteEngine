#include "scene_layer.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "application.hpp"
#include "scene_object.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"

#include "core/ecs/base.hpp"

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
        auto* base = ecs::Base::get(ent);

        _ptr           = ent;
        _depth        = 0;
        _depth_active = false;
        _origin.point1 = base->_position;
        _origin.point2 = _origin.point1;
        _bbox.x1       = _origin.point1.x;
        _bbox.y1       = _origin.point1.y;
        _bbox.x2       = _origin.point1.x;
        _bbox.y2       = _origin.point1.y;

        if (ecs::Isometric::contains(ent))
        {
            auto* iso = ecs::Isometric::get(ent);
            _origin.point1 += iso->a;
            _origin.point2 += iso->b;
        }

        if (ecs::Sprite::contains(ent))
        {
            auto* img = ecs::Sprite::get(ent);
            if (img->pack.sprite)
            {
                _bbox.x1 -= img->pack.sprite->_origina.x;
                _bbox.y1 -= img->pack.sprite->_origina.y;
                _bbox.x2 = _bbox.x1 + img->pack.sprite->_source.width;
                _bbox.y2 = _bbox.y1 + img->pack.sprite->_source.height;
            }  
        }

        _back.clear();
    }


    ObjectSceneLayer::ObjectSceneLayer() : SceneLayer(SceneLayer::Type::Object)
    {
        name() = "ObjectLayer";
        icon() = ICON_FA_MAP_PIN;
        _color = 0xffa0a0ff;
    };

    ObjectSceneLayer ::~ObjectSceneLayer()
    {
        clear();
    }

    void ObjectSceneLayer::insert(Entity ent)
    {
        if (ecs::Base::contains(ent))
        {
            auto* obj = ecs::Base::get(ent);
            if (obj->_layer == this)
                return;

            if (obj->_layer)
                obj->_layer->remove(ent);

            obj->_layer = this;
            _spatial_db.update_for_new_location(obj);
        }
        _objects.emplace(ent);
    }

    void ObjectSceneLayer::remove(Entity ent)
    {
        if (!ecs::Base::contains(ent))
            return;

        auto* obj = ecs::Base::get(ent);
        if (obj->_layer != this)
            return;

        obj->_layer = nullptr;
        _spatial_db.remove_from_bin(obj);
        _objects.erase(ent);
    }

    Entity ObjectSceneLayer::find_at(Vec2f position) const
    {
        return entt::null;
    }

    Entity ObjectSceneLayer::find_active_at(Vec2f position) const
    {
        return entt::null;
    }

    void ObjectSceneLayer::select_edit(Entity ent)
    {
        _edit = ent;
    }

    void ObjectSceneLayer::moveto(Entity ent, Vec2f pos)
    {
        auto* obj      = ecs::Base::get(ent);
        obj->_position = pos;
        _spatial_db.update_for_new_location(obj);
    }

    void ObjectSceneLayer::move(Entity ent, Vec2f pos)
    {
        auto* obj      = ecs::Base::get(ent);
        obj->_position += pos;
        _spatial_db.update_for_new_location(obj);
    }

    void ObjectSceneLayer::update(float dt)
    {
    }

    void ObjectSceneLayer::clear()
    {
        _iso_pool.clear();
        _iso.clear();
        _grid_size     = {};
        _iso_pool_size = {};
        _objects.clear();
    }

    void ObjectSceneLayer::resize(Vec2f size)
    {
        _grid_size.x = (size.width + (tile_size - 1)) / tile_size; // Round up division
        _grid_size.y = (size.height + (tile_size - 1)) / tile_size;
        _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                         _grid_size.x,
                         _grid_size.y);

        for (Entity et : _objects)
        {
            if (ecs::Base::contains(et))
            {
                auto* obj = ecs::Base::get(et);
                obj->_bin   = nullptr;
                obj->_prev  = nullptr;
                obj->_next  = nullptr;
                _spatial_db.update_for_new_location(obj);
            }
        }
    }

    void ObjectSceneLayer::activate(const Rectf& region)
    {
        SceneLayer::activate(region);

        _selected.clear();
        _iso_pool_size = 0;
        auto& reg = parent()->factory().get_registry();

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

    void ObjectSceneLayer::render(Renderer& dc)
    {
        if (is_hidden())
            return;

        dc.set_color(WHITE);

        auto& reg = parent()->factory().get_registry();
        auto sprites = reg.view<ecs::Sprite, ecs::Base>();

        for (auto ent : _iso)
        {
            if (sprites.contains(ent->_ptr))
            {
                auto* base   = ecs::Base::get(ent->_ptr);
                auto* sprite = ecs::Sprite::get(ent->_ptr);

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

    void ObjectSceneLayer::render_edit(Renderer& dc)
    {
        dc.set_color(WHITE);

        for (auto ent : _selected)
        {
            if (ecs::Base::contains(ent) && ecs::Isometric::contains(ent))
            {
                auto* base   = ecs::Base::get(ent);
                auto* iso = ecs::Isometric::get(ent);

                if (g_settings.visible_isometric)
                {
                    Vec2f a = iso->a + base->_position;
                    Vec2f b = iso->b + base->_position;
                    dc.set_color(RED);
                    dc.render_line(a, b);
                }
            }
        }
        dc.set_color(WHITE);
    }

    void ObjectSceneLayer::imgui_workspace(Params& params, DragData& drag)
    {
        _drop = entt::null;

        if (is_hidden())
            return;

        if (ImGui::IsItemClicked(0))
        {
            auto el = find_active_at(params.mouse);
            select_edit(el);
        }

        if (ImGui::IsItemClicked(1))
        {
            select_edit(entt::null);
        }

        if (drag._active && ecs::Base::contains(_edit))
        {
            move(_edit, drag._delta);
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY",
                                                                           ImGuiDragDropFlags_AcceptPeekOnly |
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                IM_ASSERT(payload->DataSize == sizeof(Entity));
                _drop = *(const Entity*)payload->Data;
                if (ecs::Base::contains(_drop))
                {
                    auto* obj = ecs::Base::get(_drop);
                    obj->_position = {params.mouse.x, params.mouse.y};
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
                if (ecs::Base::contains(_drop))
                {
                    auto* obj      = ecs::Base::get(_drop);
                    obj->_position = {params.mouse.x, params.mouse.y};

                    insert(_drop);
                }

                _drop = entt::null;
                
            }

            ImGui::EndDragDropTarget();
        }
    }

    void ObjectSceneLayer::imgui_workspace_menu()
    {
    }

    void ObjectSceneLayer::imgui_update(bool items)
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
                    remove(_edit);
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
                            select_edit(el->_ptr);
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
                                select_edit(el);
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
            parent()->factory().imgui_prefab(parent(), _edit);
        }
    }

    SceneLayer* SceneLayer::create_object()
    {
        return new ObjectSceneLayer;
    }

} // namespace fin
