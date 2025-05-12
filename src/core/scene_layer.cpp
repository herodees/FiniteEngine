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
    constexpr int32_t tile_size(512);

    void BeginDefaultMenu(const char* id)
    {
        ImGui::LineItem(ImGui::GetID(id), {-1, ImGui::GetFrameHeightWithSpacing()}).Space();
    }

    bool EndDefaultMenu()
    {
        ImGui::Line()
            .Spring()
            .PushStyle(ImStyle_Button, -100, g_settings.visible_grid)
            .Text(ICON_FA_BORDER_ALL )
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -101, g_settings.visible_isometric)
            .Text(ICON_FA_MAP_LOCATION_DOT )
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -102, g_settings.visible_collision)
            .Text(ICON_FA_VECTOR_SQUARE)
            .PopStyle();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == -100)
                g_settings.visible_grid = !g_settings.visible_grid;
            if (ImGui::Line().HoverId() == -101)
                g_settings.visible_isometric = !g_settings.visible_isometric;
            if (ImGui::Line().HoverId() == -102)
                g_settings.visible_collision = !g_settings.visible_collision;
            return true;
        }
        return false;
    }

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

        bool find_path(const IsoSceneObject* obj, Vec2i target, std::vector<Vec2i>& path) override
        {
            auto from = obj->position();
            _pathfinder.AddExternalPoints({NavMesh::Point(from.x, from.y), NavMesh::Point(target.x, target.y)});
            auto out = _pathfinder.GetPath(NavMesh::Point(from.x, from.y), NavMesh::Point(target.x, target.y));
            path.swap(reinterpret_cast<std::vector<Vec2i>&>(out));
            return out.empty();
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
                SceneLayer::render_grid(dc);
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
                if ((*it)->_ptr->bounding_box().contains(position))
                {
                    return (*it)->_ptr;
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
            obj->_id = _scene.size();
            obj->_layer = this;
            _scene.push_back(obj);
            _spatial_db.update_for_new_location(obj);
        }

        void remove(IsoSceneObject* obj)
        {
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



    /// <summary>
    /// SpriteSceneLayer
    /// </summary>
    class SpriteSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            Atlas::Pack _sprite;
            Rectf       _bbox;
            uint32_t    _index{};

            bool        operator==(const Node& ot) const
            {
                return this == &ot;
            }
        };

        SpriteSceneLayer() : SceneLayer(SceneLayer::Type::Sprite), _spatial({})
        {
            name() = "SpriteLayer";
            icon() = ICON_FA_IMAGE;
            _color = 0xffffa0b0;
        };

        void destroy(int32_t n)
        {
            _spatial.remove(_spatial[n]);
        }

        void resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void activate(const Rectf& region) override
        {
            SceneLayer::activate(region);
            _spatial.activate(region);
            if (_sort_y)
                _spatial.sort_active([&](int a, int b) { return _spatial[a]._bbox.y < _spatial[b]._bbox.y; });
            else
                _spatial.sort_active([&](int a, int b) { return _spatial[a]._index < _spatial[b]._index; });
        }

        void moveto(int obj, Vec2f pos)
        {
            auto o = _spatial[obj];
            _spatial.remove(_spatial[obj]);
            o._bbox.x = pos.x;
            o._bbox.y = pos.y;
            _spatial.insert(o);
        }

        void update(float dt) override
        {
        }

        void render(Renderer& dc) override
        {
            if (is_hidden())
                return;

            dc.set_color(WHITE);
            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];
                dc.render_texture(nde._sprite.sprite->_texture, nde._sprite.sprite->_source, nde._bbox);
            }

            dc.set_color(WHITE);
            if (_edit._sprite.sprite)
            {
                dc.render_texture(_edit._sprite.sprite->_texture, _edit._sprite.sprite->_source, _edit._bbox);
            }
        }

        void render_edit(Renderer& dc) override
        {
            if (uint32_t(_select) < (uint32_t)_spatial.size())
            {
                auto& nde = _spatial[_select];

                dc.set_color({255, 255, 255, 255});
                dc.render_line_rect(nde._bbox);
            }

            if (g_settings.visible_grid)
            {
                SceneLayer::render_grid(dc);
            }
        }

        void serialize(msg::Writer& ar) override
        {
            SceneLayer::serialize(ar);

            ar.member("max_index", _max_index);
            ar.member("sort_y", _sort_y);
            auto save = [&ar](Node& node)
            {
                ar.begin_object();
                ar.member("i", node._index);
                ar.member("a", node._sprite.atlas->get_path());
                ar.member("s", node._sprite.sprite->_name);
                ar.member("x", node._bbox.x);
                ar.member("y", node._bbox.y);
                ar.end_object();
            };

            ar.key("items");
            ar.begin_array();
            _spatial.for_each(save);
            ar.end_array();
        }

        void deserialize(msg::Value& ar) override
        {
            SceneLayer::deserialize(ar);
            _spatial.clear();
            _max_index = ar["max_index"].get(0u);
            _sort_y = ar["sort_y"].get(false);
            auto els = ar["items"];
            for (auto& el : els.elements())
            {
                Node nde;
                nde._sprite = Atlas::load_shared(el["a"].str(), el["s"].str());
                if (nde._sprite.sprite)
                {
                    nde._index       = el["i"].get(0u);
                    nde._bbox.x      = el["x"].get(0.f);
                    nde._bbox.y      = el["y"].get(0.f);
                    nde._bbox.width  = nde._sprite.sprite->_source.width;
                    nde._bbox.height = nde._sprite.sprite->_source.height;
                    _spatial.insert(nde);
                }
            }
        }

        int find_at(Vec2f position)
        {
            return _spatial.find_at(position.x, position.y);
        }

        int find_active(Vec2f position)
        {
            auto els = _spatial.get_active();

            for (auto it = els.rbegin(); it != els.rend(); ++it)
            {
                if (_spatial[*it]._bbox.contains(position))
                {
                    return *it;
                }
            }
            return -1;
        }

        void edit_active()
        {
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
            _edit._sprite = {};

            if (ImGui::IsItemClicked(0))
            {
                _select = find_active(params.mouse);
            }
            if (ImGui::IsItemClicked(1))
            {
                _select = -1;
            }

            if (drag._active && _select >= 0)
            {
                moveto(_select, drag._delta + _spatial[_select]._bbox.top_left());
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE",
                                                                               ImGuiDragDropFlags_AcceptPeekOnly |
                                                                                   ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<Atlas::Pack*>(ImGui::GetDragData("SPRITE")))
                    {
                        _edit._sprite      = *object;
                        _edit._bbox.width  = _edit._sprite.sprite->_source.width;
                        _edit._bbox.height = _edit._sprite.sprite->_source.height;
                        _edit._bbox.x      = params.mouse.x;
                        _edit._bbox.y      = params.mouse.y;
                        _edit._index       = _max_index;
                    }
                }
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE",
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<Atlas::Pack*>(ImGui::GetDragData("SPRITE")))
                    {
                        _edit._sprite      = *object;
                        _edit._bbox.width  = _edit._sprite.sprite->_source.width;
                        _edit._bbox.height = _edit._sprite.sprite->_source.height;
                        _edit._bbox.x      = params.mouse.x;
                        _edit._bbox.y      = params.mouse.y;
                        _edit._index       = _max_index;
                        ++_max_index;
                        _spatial.insert(_edit);
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }

        void imgui_setup() override
        {
            SceneLayer::imgui_setup();
            ImGui::Checkbox("Sort by Y", &_sort_y);
        }

        void imgui_update(bool items) override
        {
            if (!items)
                return edit_active();

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
                if (_select == -1 && ImGui::Line().HoverId() == 1)
                {
                    destroy(_select);
                    _select = -1;
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
                    for (auto n : _spatial.get_active())
                    {
                        ImGui::PushID(n);
                        auto& el = _spatial[n];
                        if (el._sprite.sprite)
                        {
                            if (ImGui::Selectable(el._sprite.sprite->_name.c_str(), n == _select))
                            {
                                _select = n;
                            }
                        }
                        ImGui::PopID();
                    }
                }
                else
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(_spatial.size());
                    while (clipper.Step())
                    {
                        for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                        {
                            ImGui::PushID(n);
                            auto& el = _spatial[n];
                            if (el._sprite.sprite)
                            {
                                if (ImGui::Selectable(el._sprite.sprite->_name.c_str(), n == _select))
                                {
                                    _select = n;
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                }
            }
            ImGui::EndChildFrame();
        }

    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        Node                                                                                 _edit;
        int32_t                                                                              _select = -1;
        uint32_t                                                                             _max_index = 0;
        bool                                                                                 _sort_y = false;
    };



    /// <summary>
    /// RegionSceneLayer
    /// </summary>
    class RegionSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            msg::Var _points;
            Rectf    _bbox;
            bool     _need_update{};
            bool     operator==(const Node& ot) const
            {
                return this == &ot;
            }

            uint32_t size() const
            {
                return _points.size() / 2;
            }

            Vec2f get_point(uint32_t n)
            {
                return {_points[n * 2].get(0.f), _points[n * 2 + 1].get(0.f)};
            }

            void set_point(uint32_t n, Vec2f val)
            {
                _need_update = true;
                _points.set_item(n * 2, val.x);
                _points.set_item(n * 2 + 1, val.y);
            }

            Node& insert(Vec2f pt, int32_t n)
            {
                _need_update = true;
                if (uint32_t(n * 2) >= _points.size())
                {
                    _points.push_back(pt.x);
                    _points.push_back(pt.y);
                }
                else
                {
                    n = n * 2;
                    _points.insert(n, pt.x);
                    _points.insert(n + 1, pt.y);
                }
                return *this;
            }

            int32_t find(Vec2f pt, float radius)
            {
                auto sze = _points.size();
                for (uint32_t n = 0; n < sze; n += 2)
                {
                    Vec2f pos(_points.get_item(n).get(0.f), _points.get_item(n + 1).get(0.f));
                    if (pos.distance_squared(pt) <= (radius * radius))
                    {
                        return n / 2;
                    }
                }
                return -1;
            }

            void update()
            {
                _need_update = false;
                _bbox = {};
                if (_points.size() >= 2)
                {
                    Region<float> reg(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
                    for (uint32_t n = 0; n < _points.size(); n += 2)
                    {
                        const float nxo = _points.get_item(n).get(0.f);
                        const float nyo = _points.get_item(n + 1).get(0.f);

                        if (nxo < reg.x1)
                            reg.x1 = nxo;

                        if (nxo > reg.x2)
                            reg.x2 = nxo;

                        if (nyo < reg.y1)
                            reg.y1 = nyo;

                        if (nyo > reg.y2)
                            reg.y2 = nyo;
                    }

                    _bbox = reg.rect();
                }
            }
        };

        RegionSceneLayer() : SceneLayer(SceneLayer::Type::Region), _spatial({})
        {
            name() = "RegionLayer";
            icon() = ICON_FA_MAP_LOCATION_DOT;
            _color = 0xff50ffc0;
        };

        void resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void activate(const Rectf& region) override
        {
            SceneLayer::activate(region);
            _spatial.activate(region);
        }

        void serialize(msg::Writer& ar)
        {
            SceneLayer::serialize(ar);

            auto save = [&ar](Node& node)
            {
                ar.begin_object();
                ar.key("p");
                ar.begin_array();
                for (auto el : node._points.elements())
                    ar.value(el.get(0.f));
                ar.end_array();
                ar.end_object();
            };

            ar.key("items");
            ar.begin_array();
            _spatial.for_each(save);
            ar.end_array();
        }

        void deserialize(msg::Value& ar)
        {
            SceneLayer::deserialize(ar);
            _spatial.clear();

            auto els = ar["items"];
            for (auto& el : els.elements())
            {
                Node nde;
                auto pts = el["p"];
                for (auto& el : pts.elements())
                {
                    nde._points.push_back(el.get(0.f));
                }
                nde.update();
                _spatial.insert(nde);
            }
        }

        int find_at(Vec2f position)
        {
            return _spatial.find_at(position.x, position.y);
        }

        void moveto(int obj, Vec2f pos)
        {
            auto o = _spatial[obj];
            _spatial.remove(_spatial[obj]);
            if (o._points.size() >= 2)
            {
                const float xo = o._points.get_item(0).get(0.f);
                const float yo = o._points.get_item(1).get(0.f);

                for (uint32_t n = 0; n < o._points.size(); n += 2)
                {
                    const float nxo = o._points.get_item(n).get(0.f) - xo + pos.x;
                    const float nyo = o._points.get_item(n + 1).get(0.f) - yo + pos.y;

                    o._points.set_item(n, nxo);
                    o._points.set_item(n + 1, nyo);
                }
            }
            o.update();

            _spatial.insert(o);
        }

        void render_edit(Renderer& dc) override
        {
            dc.set_color(WHITE);

            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];

                auto sze = nde.size();
                for (auto idx = 0; idx < sze; ++idx)
                {
                    dc.render_line(nde.get_point(idx), nde.get_point((idx + 1) % sze));
                }

            }


            if (auto* reg = selected_region())
            {
                dc.set_color({255, 255, 0, 255});
                dc.render_line_rect(reg->_bbox);

                for (auto n = 0; n < reg->size(); ++n)
                {
                    auto pt = reg->get_point(n);
                    if (_active_point == n)
                        dc.render_circle(pt, 3);
                    else
                        dc.render_line_circle(pt, 3);
                }
            }

            if (g_settings.visible_grid)
            {
                SceneLayer::render_grid(dc);
            }
        }

        void update(float dt) override
        {
        }

        void imgui_workspace_menu() override
        {
            BeginDefaultMenu("wsmnu");
            ImGui::Line()
                .PushStyle(ImStyle_Button, 10, _edit_region)
                .Text(ICON_FA_ARROW_POINTER " Edit")
                .PopStyle()
                .Space()
                .PushStyle(ImStyle_Button, 20, !_edit_region)
                .Text(ICON_FA_BRUSH " Create")
                .PopStyle();

            if (EndDefaultMenu())
            {
                if (ImGui::Line().HoverId() == 10)
                {
                    _edit_region = true;
                }
                if (ImGui::Line().HoverId() == 20)
                {
                    _edit_region = false;
                }
            }
        }

        void imgui_workspace(Params& params, DragData& drag) override
        {
            // Edit region points
            if (_edit_region)
            {
                if (auto* reg = selected_region())
                {
                    if (ImGui::IsItemClicked(0))
                    {
                        _active_point = reg->find(params.mouse, 5);
                    }
                    if (ImGui::IsItemClicked(1))
                    {
                        _selected = -1;
                    }
                }
                else if (ImGui::IsItemClicked(0))
                {
                    _selected = find_at(params.mouse);
                }

                if (drag._active && _active_point != -1)
                {
                    if (auto* reg = selected_region())
                    {
                        auto obj = *reg;
                        _spatial.remove(*selected_region());
                        auto pt = obj.get_point(_active_point) + drag._delta;
                        obj.set_point(_active_point, pt);
                        obj.update();
                        _spatial.insert(obj);
                    }
                }
            }
            // Create region
            else
            {
                if (ImGui::IsItemClicked(0))
                {
                    if (auto *reg = selected_region())
                    {
                        auto obj = *reg;
                        _spatial.remove(*reg);
                        obj.insert(params.mouse, _active_point + 1);
                        obj.update();
                        _spatial.insert(obj);
                        _active_point = _active_point + 1;
                    }
                    else
                    {
                        Node obj;
                        obj.insert(params.mouse, 0);
                        obj.update();
                        _selected = _spatial.insert(obj);
                        _active_point = 0;
                    }
                }
                if (ImGui::IsItemClicked(1))
                {
                    _active_point = -1;
                    _selected     = -1;
                }
            }
        }

        void edit_active()
        {
            if (auto* reg = selected_region())
            {
            }
        }

        void imgui_update(bool items) override
        {
            if (!items)
                return edit_active();

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
                if (ImGui::Line().HoverId() == 1)
                {
                    if (auto* reg = selected_region())
                    {
                        _spatial.remove(*reg);
                    }
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
                    for (auto n : _spatial.get_active())
                    {
                        ImGui::PushID(n);
                        auto& el = _spatial[n];
                        if (el._points.size())
                        {
                            if (ImGui::Selectable(ImGui::FormatStr("Region [%d]", el._points.size() / 2), n == _selected))
                            {
                                _selected = n;
                            }
                        }
                        ImGui::PopID();
                    }
                }
                else
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(_spatial.size());
                    while (clipper.Step())
                    {
                        for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                        {
                            ImGui::PushID(n);
                            auto& el = _spatial[n];
                            if (el._points.size())
                            {
                                if (ImGui::Selectable(ImGui::FormatStr("Region [%d]", el._points.size() / 2), n == _selected))
                                {
                                    _selected = n;
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                }
            }

            ImGui::EndChildFrame();
        }
    private:
        Node* selected_region()
        {
            if (size_t(_selected) < size_t(_spatial.size()))
            {
                return &_spatial[_selected];
            }
            return nullptr;
        }

        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        int32_t                                                                              _edit = -1;
        int32_t                                                                              _selected = -1;
        int32_t                                                                              _active_point = -1;
        bool                                                                                 _edit_region{};
    };



    SceneLayer* SceneLayer::create(msg::Value& ar, Scene* scene)
    {
        if (auto* obj = create(Type(ar["type"].get(0))))
        {
            obj->_parent = scene;
            obj->deserialize(ar);
            return obj;
        }
        return nullptr;
    }

    SceneLayer* SceneLayer::create(Type t)
    {
        switch (t)
        {
            case Type::Sprite:
                return new SpriteSceneLayer;
            case Type::Region:
                return new RegionSceneLayer;
            case Type::Isometric:
                return new IsometricSceneLayer;
        }
        return nullptr;
    }

    SceneLayer::SceneLayer(Type t) : _type(t)
    {
    }

    SceneLayer::Type SceneLayer::type() const
    {
        return _type;
    }

    std::string& SceneLayer::name()
    {
        return _name;
    }

    std::string_view& SceneLayer::icon()
    {
        return _icon;
    }

    uint32_t SceneLayer::color() const
    {
        return _color;
    }

    Scene* SceneLayer::parent()
    {
        return _parent;
    }

    const Rectf& SceneLayer::region() const
    {
        return _region;
    }

    Vec2f SceneLayer::screen_to_view(Vec2f pos) const
    {
        return Vec2f(pos.x + _region.x, pos.y + _region.y); 
    }

    Vec2f SceneLayer::view_to_screen(Vec2f pos) const
    {
        return Vec2f(pos.x - _region.x, pos.y - _region.y); 
    }

    Vec2f SceneLayer::get_mouse_position() const
    {
        const auto p{GetMousePosition()};
        return {p.x + _region.x, p.y + _region.y};
    }

    void SceneLayer::serialize(msg::Writer& ar)
    {
        ar.member("type", (int)_type);
        ar.member("name", _name);
    }

    void SceneLayer::deserialize(msg::Value& ar)
    {
        _type = Type(ar["type"].get(0));
        _name = ar["name"].str();
    }

    void SceneLayer::resize(Vec2f size)
    {
    }

    void SceneLayer::clear()
    {
    }

    void SceneLayer::activate(const Rectf& region)
    {
        _region = region;
    }

    void SceneLayer::update(float dt)
    {
    }

    void SceneLayer::render(Renderer& dc)
    {
    }

    void SceneLayer::render_edit(Renderer& dc)
    {
    }

    bool SceneLayer::find_path(const IsoSceneObject* obj, Vec2i target, std::vector<Vec2i>& path)
    {
        return false;
    }

    void SceneLayer::moveto(IsoSceneObject* obj, Vec2f pos)
    {
    }

    void SceneLayer::imgui_update(bool items)
    {
    }

    void SceneLayer::imgui_setup()
    {
        ImGui::InputText("Name", &_name);
    }

    void SceneLayer::imgui_workspace(Params& params, DragData& drag)
    {
    }

    void SceneLayer::imgui_workspace_menu()
    {
    }

    bool SceneLayer::is_hidden() const
    {
        return _hidden;
    }

    bool SceneLayer::is_active() const
    {
        return _active;
    }

    void SceneLayer::hide(bool b)
    {
        _hidden = b;
    }

    void SceneLayer::activate(bool a)
    {
        _active = a;
    }

    void SceneLayer::render_grid(Renderer& dc)
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
            dc.render_line((float)minpos.x * tile_size, (float)y * tile_size, (float)maxpos.x * tile_size, (float)y * tile_size);
        }

        for (int x = minpos.x; x < maxpos.x; ++x)
        {
            dc.render_line((float)x * tile_size, (float)minpos.y * tile_size, (float)x * tile_size, (float)maxpos.y * tile_size);
        }
    }


} // namespace fin
