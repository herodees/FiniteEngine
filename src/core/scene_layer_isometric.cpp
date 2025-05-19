#include "scene_layer.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "application.hpp"
#include "scene_object.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{
    class IsometricSceneLayer : public SceneLayer
    {
        struct IsoObject
        {
            int32_t                 _depth        : 31;
            bool                    _depth_active : 1;
            Line<float>             _origin;
            Region<float>           _bbox;
            IsoSceneObject*         _ptr;
            std::vector<IsoObject*> _back;
            int32_t                 depth_get()
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
        };

    public:
        IsometricSceneLayer() : SceneLayer(SceneLayer::Type::Isometric)
        {
            name() = "IsometricLayer";
            icon() = ICON_FA_MAP_PIN;
            _color = 0xffa0a0ff;
        };

        ~IsometricSceneLayer() override
        {
            clear();
        }

        void init() override
        {
            for (auto* el : _scene)
            {
                el->init();
            }
        }

        void deinit() override
        {
        }

        std::span<const Vec2i> find_path(const IsoSceneObject* obj, Vec2i target) override
        {
            Vec2i          from  = obj->position();
            NavMesh::Point pts[] = {{from.x, from.y}, {target.x, target.y}};
            _pathfinder.AddExternalPoints(pts);
            return (std::span<const Vec2i>&)(_pathfinder.GetPath(_pathfinder.GetExternalPoints()[0],
                                                                 _pathfinder.GetExternalPoints()[1]));
        }

        void update_navmesh()
        {
            std::vector<NavMesh::Polygon> polygons;

            for (auto el : _scene)
            {
                auto& col = el->collision();
                if (col.size())
                {
                    auto& poly = polygons.emplace_back();
                    for (uint32_t n = 0; n < col.size(); n += 2)
                    {
                        NavMesh::Point pt(col.get_item(n).get(0.f), col.get_item(n + 1).get(0.f));
                        pt.x += el->position().x;
                        pt.y += el->position().y;
                        poly.AddPoint(pt);
                    }
                }
            }
            _pathfinder.AddPolygons(polygons, _inflate);
        }

        void clear() override
        {
            std::for_each(_scene.begin(), _scene.end(), [](auto* p) { delete p; });
            _scene.clear();
            _iso_pool.clear();
            _iso.clear();
            _grid_size     = {};
            _iso_pool_size = {};
            _edit          = {};
            _select        = {};
        }

        void resize(Vec2f size) override
        {
            _grid_size.x = (size.width + (tile_size - 1)) / tile_size; // Round up division
            _grid_size.y = (size.height + (tile_size - 1)) / tile_size;
            _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                             _grid_size.x,
                             _grid_size.y);

            for (auto* s : _scene)
            {
                s->_bin  = nullptr;
                s->_prev = nullptr;
                s->_next = nullptr;

                _spatial_db.update_for_new_location(s);
            }
        }

        void activate(const Rectf& region) override
        {
            SceneLayer::activate(region);

            _iso_pool_size = 0;

            auto cb = [&](lq::SpatialDatabase::Proxy* item)
            {
                if (_iso_pool_size >= _iso_pool.size())
                    _iso_pool.resize(_iso_pool_size + 32);

                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(item);
                ++_iso_pool_size;
            };

            auto add_pool_item = [&](size_t n)
            {
                auto& obj                  = *_iso_pool[n]._ptr;
                _iso[n]                    = &_iso_pool[n];
                _iso_pool[n]._depth        = 0;
                _iso_pool[n]._depth_active = false;
                _iso_pool[n]._back.clear();
                _iso_pool[n]._bbox   = obj.bounding_box();
                _iso_pool[n]._origin = obj.iso();
                obj.flag_reset(SceneObjectFlag::Marked);
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
                add_pool_item(n);
            }

            // Add edit object to render queue
            if (_edit)
            {
                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(_edit);
                _iso.emplace_back();
                add_pool_item(_iso_pool_size);
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

        void update(float dt) override
        {
            for (auto* obj : _iso)
            {
                if (!obj->_ptr->is_disabled())
                    obj->_ptr->update(dt);
            }
        }

        void render(Renderer& dc) override
        {
            if (is_hidden())
                return;

            dc.set_color(WHITE);
            for (auto* obj : _iso)
            {
                if (!obj->_ptr->is_hidden())
                    obj->_ptr->render(dc);
            }
        }

        void render_edit(Renderer& dc) override
        {
            for (auto* obj : _iso)
            {
                obj->_ptr->edit_render(dc, _select == obj->_ptr);
            }

            if (g_settings.visible_grid)
            {
                render_grid(dc);
            }
        }

        void render_grid(Renderer& dc)
        {
            const int startX = std::max(0.f, _region.x / tile_size);
            const int startY = std::max(0.f, _region.y / tile_size);

            const int endX = (_region.x2() / tile_size) + 2;
            const int endY = (_region.y2() / tile_size) + 2;

            auto minpos = Vec2i{startX, startY};
            auto maxpos = Vec2i{endX, endY};

            Color clr{255, 255, 0, 190};
            dc.set_color(clr);

            for (int y = minpos.y; y < maxpos.y; ++y)
            {
                dc.render_line((float)minpos.x * tile_size,
                               (float)y * tile_size,
                               (float)maxpos.x * tile_size,
                               (float)y * tile_size);
            }

            for (int x = minpos.x; x < maxpos.x; ++x)
            {
                dc.render_line((float)x * tile_size,
                               (float)minpos.y * tile_size,
                               (float)x * tile_size,
                               (float)maxpos.y * tile_size);
            }
        }

        void object_serialize(IsoSceneObject* obj, msg::Writer& ar)
        {
            ar.begin_object();
            obj->serialize(ar);
            if (obj->is_named())
            {
                ar.member(Sc::Name, obj->_name);
            }
            ar.end_object();
        }

        IsoSceneObject* object_deserialize(msg::Value& ar)
        {
            auto ot = ar[Sc::Uid].get(0ull);
            if (IsoSceneObject* obj = SceneFactory::instance().create(ot))
            {
                obj->deserialize(ar);
                auto id = ar[Sc::Name].str();
                if (!id.empty())
                {
                    parent()->name_object(obj, id);
                }
                return static_cast<IsoSceneObject*>(obj);
            }
            return nullptr;
        }

        void serialize(msg::Writer& ar) override
        {
            SceneLayer::serialize(ar);
            ar.member("inflate", _inflate);
            ar.key("items");
            ar.begin_array();
            for (auto* el : _scene)
            {
                object_serialize(el, ar);
            }
            ar.end_array();
        }

        void deserialize(msg::Value& ar) override
        {
            SceneLayer::deserialize(ar);
            _inflate = ar["inflate"].get(0);
            auto els = ar["items"];
            for (auto el : els.elements())
            {
                if (auto* obj = object_deserialize(el))
                {
                    insert(obj);
                }
                else
                {
                    TraceLog(LOG_ERROR, "Isometric Scene Layer: Object load failed!");
                }
            }
            update_navmesh();
        }

        void imgui_workspace_menu() override
        {
            BeginDefaultMenu("wsmnu");
            if (EndDefaultMenu())
            {
            }
        }

        void imgui_workspace(Params& params, DragData& drag) override
        {
            _edit = nullptr;

            if (is_hidden())
                return;

            if (ImGui::IsItemClicked(0))
            {
                if (auto* obj = find_active(params.mouse))
                {
                    select(obj);
                }
                else
                {
                    select(nullptr);
                }
            }
            if (ImGui::IsItemClicked(1))
            {
                select(nullptr);
            }

            if (drag._active && _select)
            {
                moveto(_select, drag._delta + _select->position());
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB",
                                                                               ImGuiDragDropFlags_AcceptPeekOnly |
                                                                                   ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<IsoSceneObject*>(ImGui::GetDragData("PREFAB")))
                    {
                        _edit            = object;
                        _edit->_position = {params.mouse.x, params.mouse.y};
                    }
                }
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB",
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<IsoSceneObject*>(ImGui::GetDragData("PREFAB")))
                    {

                        object->_position = {params.mouse.x, params.mouse.y};
                        msg::Pack doc;
                        auto      ar = doc.create();
                        object_serialize(object, ar);
                        auto arr = doc.get();
                        insert(object_deserialize(arr));
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }

        void edit_active()
        {
            if (_select)
            {
                g_settings.buffer = _select->is_named() ? _select->_name : "";
                if (ImGui::InputText("Name", &g_settings.buffer))
                {
                    parent()->name_object(_select, g_settings.buffer);
                }

                _select->imgui_update();
            }
        }

        void imgui_update(bool items) override
        {
            if (!items)
                return edit_active();

            ImGui::InputInt("Inflate", &_inflate);

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
                if (_select && ImGui::Line().HoverId() == 1)
                {
                    destroy(_select);
                    _select = nullptr;
                    return;
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    g_settings.list_visible_items = !g_settings.list_visible_items;
                }
            }

            if (ImGui::BeginChildFrame(-1, {-1, 250}, 0))
            {
                if (g_settings.list_visible_items)
                {
                    for (auto& el : _iso)
                    {
                        ImGui::PushID(el->_ptr);
                        const char* name = el->_ptr->_name;
                        if (!name)
                            name = ImGui::FormatStr("Object %p", el->_ptr);

                        if (ImGui::Selectable(name, el->_ptr == _select))
                        {
                            select(el->_ptr);
                        }
                        ImGui::PopID();
                    }
                }
                else
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(_scene.size());
                    while (clipper.Step())
                    {
                        for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                        {
                            ImGui::PushID(n);
                            auto*       el   = _scene[n];
                            const char* name = el->_name;
                            if (!name)
                                name = ImGui::FormatStr("Object %p", el);

                            if (ImGui::Selectable(name, el == _select))
                            {
                                select(el);
                            }
                            ImGui::PopID();
                        }
                    }
                }
            }

            ImGui::EndChildFrame();
        }

        IsoSceneObject* find_active(Vec2f position)
        {
            for (auto it = _iso.rbegin(); it != _iso.rend(); ++it)
            {
                auto* obj  = (*it)->_ptr;
                auto  bbox = obj->bounding_box();

                if (bbox.contains(position))
                {
                    if (obj->sprite_object())
                    {
                        auto& spr = static_cast<SpriteSceneObject*>(obj)->sprite();
                        if (spr.is_alpha_visible(position.x - bbox.x1, position.y - bbox.y1))
                        {
                            return obj;
                        }
                    }
                    else
                    {
                        return (*it)->_ptr;
                    }
                }
            }
            return nullptr;
        }

        IsoSceneObject* find_at(Vec2f position, float radius)
        {
            struct
            {
                lq::SpatialDatabase::Proxy* obj  = nullptr;
                float                       dist = FLT_MAX;
            } out;

            auto cb = [&out, position](lq::SpatialDatabase::Proxy* obj, float dist)
            {
                auto*         object = static_cast<SpriteSceneObject*>(obj);
                Region<float> bbox;
                Line<float>   line;
                if (object->bounding_box().contains(position))
                {
                    out.obj = obj;
                }
            };

            _spatial_db.map_over_all_objects_in_locality(position.x, position.y, radius, cb);
            return static_cast<SpriteSceneObject*>(out.obj);
        }

        void select(IsoSceneObject* obj)
        {
            if (_select != obj)
            {
                _select = obj;
            }
        }

        void insert(IsoSceneObject* obj)
        {
            if (!obj)
                return;
            obj->_id    = _scene.size();
            obj->_layer = this;
            _scene.push_back(obj);
            _spatial_db.update_for_new_location(obj);
            obj->init();
        }

        void remove(IsoSceneObject* obj)
        {
            obj->deinit();
            if (obj->is_named())
            {
                parent()->name_object(obj, {});
            }
            _spatial_db.remove_from_bin(obj);
            obj->_layer     = nullptr;
            const auto id   = obj->_id;
            _scene[id]      = _scene.back();
            _scene[id]->_id = id;
            _scene.pop_back();
        }

        void destroy(IsoSceneObject* obj)
        {
            if (obj)
            {
                remove(obj);
            }
            delete obj;
        }

        void moveto(IsoSceneObject* obj, Vec2f pos)
        {
            obj->_position = pos;
            _spatial_db.update_for_new_location(obj);
        }

    private:
        Vec2i                        _grid_size;
        NavMesh::PathFinder          _pathfinder;
        lq::SpatialDatabase          _spatial_db;
        std::vector<IsoSceneObject*> _scene;
        std::vector<IsoObject>       _iso_pool;
        std::vector<IsoObject*>      _iso;
        uint32_t                     _iso_pool_size{};
        int32_t                      _inflate{};
        IsoSceneObject*              _edit{};
        IsoSceneObject*              _select{};
    };

    SceneLayer* SceneLayer::create_isometric()
    {
        return new IsometricSceneLayer;
    }

} // namespace fin
