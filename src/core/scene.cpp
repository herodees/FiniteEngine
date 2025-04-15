#include "scene.hpp"
#include "atlas_scene_object.hpp"
#include "proto_scene_object.hpp"

#include "utils/dialog_utils.hpp"
#include "utils/imgui_utils.hpp"

namespace fin
{
    constexpr int32_t tile_size(512);

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
    }

    bool Scene::setup_background_texture(const std::filesystem::path& file)
    {
        Surface sf;
        if (!sf.load_from_file(file))
            return false;

        int surface_width = sf.get_width();
        int surface_height = sf.get_height();

        _grid_size.x = (surface_width + (tile_size - 1)) / tile_size;  // Round up division
        _grid_size.y = (surface_height + (tile_size - 1)) / tile_size;

        _grid_active.clear();
        _grid_texture.clear();
        _grid_surface.clear();
        _grid_surface.reserve(_grid_size.x * _grid_size.y);

        for (int y = 0; y < surface_height; y += tile_size) {
            for (int x = 0; x < surface_width; x += tile_size)
            {
                Surface tile = sf.create_sub_surface(x, y, tile_size, tile_size);
                _grid_surface.push_back(std::move(tile));
            }
        }
        _grid_texture.resize(_grid_surface.size());

        _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                         _grid_size.x,
                         _grid_size.y);
        return true;
    }

    const float& getX_V2d(const Vec2f& v) { return v.x; }
    const float& getY_V2d(const Vec2f& v) { return v.y; }


    void Scene::activate_grid(const Recti& screen)
    {
        _active_region = screen;

        auto canvas_size = _canvas.get_size();
        auto new_canvas_size = _active_region.size();
        if (canvas_size != new_canvas_size)
        {
            _canvas.create(new_canvas_size.x, new_canvas_size.y);
        }

        std::for_each(_grid_active.begin(), _grid_active.end(), [](std::pair<size_t, bool>& val) { val.second = false; });
        auto add_index = [&](size_t n)
            {
                for (auto& el : _grid_active)
                {
                    if (el.first == n) {
                        el.second = true;
                        return;
                    }
                }
                _grid_active.emplace_back(n, true);
            };

        // Step 1: Determine visible grid range
        auto start = get_active_grid_min();
        auto end = get_active_grid_max();

        // Step 2: Mark which tiles should remain active
        size_t activeIndex = 0;
        for (int y = start.y; y < end.y; ++y)
        {
            for (int x = start.x; x < end.x; ++x)
            {
                size_t index = y * _grid_size.x + x;
                add_index(index);
            }
        }

        // Step 3: Unload textures that are no longer visible
        for (size_t n = 0; n < _grid_active.size();)
        {
            if (!_grid_active[n].second)
            {
                _grid_texture[_grid_active[n].first].clear();
                std::swap(_grid_active[n], _grid_active.back());
                _grid_active.pop_back();
            }
            else
            {
                if (!_grid_texture[_grid_active[n].first])
                {
                    _grid_texture[_grid_active[n].first].load_from_surface(_grid_surface[_grid_active[n].first]);
                }
                ++n;
            }
        }
    }

    Vec2i Scene::get_active_grid_min() const
    {
        const int startX = std::max(0, _active_region.x / tile_size);
        const int startY = std::max(0, _active_region.y / tile_size);
        return Vec2i(startX, startY);
    }

    Vec2i Scene::get_active_grid_max() const
    {
        const int endX = std::min(_grid_size.x, (_active_region.x + _active_region.width) / tile_size + 1);
        const int endY = std::min(_grid_size.y, (_active_region.y + _active_region.height) / tile_size + 1);
        return Vec2i(endX, endY);
    }

    Vec2i Scene::get_scene_size() const
    {
        return Vec2i(tile_size * _grid_size.width, tile_size * _grid_size.height);
    }

    Vec2i Scene::get_grid_size() const
    {
        return _grid_size;
    }

    void Scene::object_serialize(SceneObject* obj, msg::Writer& ar)
    {
        ar.begin_object();
        ar.member("ot", obj->object_type());
        obj->serialize(ar);
        ar.end_object();
    }

    SceneObject* Scene::object_deserialize(msg::Value& ar)
    {
        auto ot = ar["ot"].get(std::string_view());
        SceneObject* obj = nullptr;
        
        if (ot == AtlasSceneObject::type_id)
            obj = new AtlasSceneObject();
        if (ot == ProtoSceneObject::type_id)
            obj = new ProtoSceneObject();

        if (obj)
            obj->deserialize(ar);

        return obj;
    }

    void Scene::object_insert(SceneObject* obj)
    {
        obj->_id = _scene.size();
        _scene.push_back(obj);
        _spatial_db.update_for_new_location(obj);
    }

    void Scene::object_remove(SceneObject* obj)
    {
        _spatial_db.remove_from_bin(obj);
        const auto id = obj->_id;
        _scene[id] = _scene.back();
        _scene[id]->_id = id;
    }

    void Scene::object_select(SceneObject *obj)
    {
        if (_selected_object != obj)
        {
            _selected_object = obj;
        }
    }

    SceneObject *Scene::object_find_at(Vec2f position, float radius)
    {
        struct
        {
            lq::SpatialDatabase::Proxy *obj = nullptr;
            float dist = FLT_MAX;
        } out;

        auto cb = [&out, position](lq::SpatialDatabase::Proxy *obj, float dist) {
            auto *object = static_cast<SceneObject *>(obj);
            Region<float> bbox;
            Line<float> line;
            object->get_iso(bbox, line);
            if (bbox.contains(position))
            {
                out.obj = obj;
            }
        };

        _spatial_db.map_over_all_objects_in_locality(position.x, position.y, radius, cb);
        return static_cast<SceneObject *>(out.obj);
    }

    void Scene::render(Renderer& dc)
    {
        if (!_canvas)
            return;

        // Draw on the canvas
        BeginTextureMode(*_canvas.get_texture());
        ClearBackground({0, 0, 0, 255});

        auto minpos = get_active_grid_min();
        auto maxpos = get_active_grid_max();

        dc.set_origin({(float)_active_region.x, (float)_active_region.y});

        BeginMode2D(dc._camera);

        dc.set_color(WHITE);
        for (int y = minpos.y; y < maxpos.y; ++y)
        {
            for (int x = minpos.x; x < maxpos.x; ++x)
            {
                auto& txt = _grid_texture[y * _grid_size.width + x];
                if (txt)
                {
                    dc.render_texture(txt.get_texture(), { 0, 0, tile_size, tile_size }, { (float)x * tile_size, (float)y * tile_size ,
                        (float)tile_size, (float)tile_size });
                }
            }
        }

        dc.set_color(WHITE);
        for (auto *obj : _iso_manager._iso)
        {
            if (!obj->_ptr->is_hidden())
                obj->_ptr->render(dc);
        }


        if (_debug_draw_object)
        {
            dc.set_color({255, 0, 0, 255});

            for (auto *obj : _iso_manager._iso)
            {
                dc.render_line(obj->_origin.point1, obj->_origin.point2);
                dc.render_debug_text({obj->_origin.point1.x + 16, obj->_origin.point1.y + 16},
                                     "%d",
                                     obj->_depth);
            }
        }

        if (_selected_object)
        {
            _selected_object->render_edit(dc);
        }

        if (_debug_draw_navmesh)
        {
            Color clr(128, 128, 128, 255);
            dc.set_color(clr);

            auto edges = _pathfinder.GetEdgesForDebug();
            for (auto& el : edges)
            {
                dc.render_line(el.b, el.e);
            }
        }

        if (_debug_draw_grid)
        {
            Color clr{255, 255, 0, 255};
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

        EndMode2D();
        EndTextureMode();
    }

    void Scene::update(float dt)
    {
        _iso_manager.update(_spatial_db, _active_region, _edit_object);
    }



    void Scene::clear()
    {
        std::for_each(_scene.begin(), _scene.end(), [](SceneObject* p) { delete p; });
        _grid_size = {};
        _grid_active.clear();
        _grid_texture.clear();
        _scene.clear();
        _spatial_db.init({}, 0, 0);
        _grid_surface.clear();
        _grid_texture.clear();
    }

    void Scene::serialize(msg::Pack& out)
    {
        auto ar = out.create();
        ar.begin_object();
        {
            ar.member("width", _grid_size.x);
            ar.member("height", _grid_size.y);

            ar.key("background");
            ar.begin_array();
            {
                for (auto& txt : _grid_surface)
                {
                    ar.begin_object();
                    ar.member("w", txt.get_surface()->width);
                    ar.member("h", txt.get_surface()->height);
                    ar.member("f", (int32_t)txt.get_surface()->format);

                    int sze{};
                    auto* dta = CompressData(txt.data(), txt.get_data_size(), &sze);
                    ar.key("d").data_value(dta, sze);
                    MemFree(dta);

                    ar.end_object();
                }
            }
            ar.end_array();

            ar.key("objects");
            ar.begin_array();
            {
                for (auto* obj : _scene)
                {
                    object_serialize(obj, ar);
                }
            }
            ar.end_array();
        }
        ar.end_object();
    }

    void Scene::deserialize(msg::Value& ar)
    {
        clear();

        _grid_size.x = ar["width"].get(0);
        _grid_size.y = ar["height"].get(0);

        _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                         _grid_size.x,
                         _grid_size.y);

        _grid_surface.reserve(_grid_size.x * _grid_size.y);

        auto els = ar["background"].elements();
        for (auto el : els)
        {
            auto d = el["d"].data_str();
            int sze{};
            auto* dta = DecompressData((const unsigned char*)d.data(), d.size(), &sze);
            _grid_surface.emplace_back()
                .load_from_pixels(el["w"].get(0), el["h"].get(0),
                                                          el["f"].get(0),
                                                          dta);
            MemFree(dta);
        }

        auto obs = ar["objects"].elements();
        for (auto el : obs)
        {
            object_insert(object_deserialize(el));
        }

        _grid_texture.resize(_grid_surface.size());
    }

    void Scene::open()
    {
        auto files = open_file_dialog("", "");
        if (!files.empty())
        {
            msg::Pack pack;
            int size{};
            auto *txt = LoadFileData(files[0].c_str(), &size);
            pack.data().assign(txt, txt + size);
            UnloadFileData(txt);
            auto ar = pack.get();
            deserialize(ar);
        }
    }

    void Scene::save()
    {
        auto out = save_file_dialog("", "");
        if (!out.empty())
        {
            msg::Pack pack;
            serialize(pack);
            auto ar = pack.data();
            SaveFileData(out.c_str(), pack.data().data(), pack.data().size());
        }
    }

    void Scene::on_imgui_props()
    {
        switch (_mode)
        {
        case Mode::Map:
            on_imgui_props_map();
            break;
        case Mode::Navmesh:
            on_imgui_props_navmesh();
            break;
        case Mode::Objects:
            on_imgui_props_object();
            break;
        }
    }

    void Scene::on_imgui_props_navmesh()
    {
        if (ImGui::CollapsingHeader("Navmesh", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button(" Delete "))
            {
            }

            if (ImGui::BeginChildFrame(-1, { -1, -1 }, 0))
            {

            }
            ImGui::EndChildFrame();
        }
    }

    void Scene::on_imgui_props_object()
    {
        if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginChildFrame(-1, { -1, 250 }, 0))
            {
                ImGuiListClipper clipper;
                clipper.Begin(_scene.size());
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        auto *el = _scene[n];
                        if (ImGui::Selectable(ImGui::FormatStr("%p", el), el == _selected_object))
                        {
                            object_select(el);
                        }
                    }
                }
            }
            ImGui::EndChildFrame();
        }

        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (_selected_object)
            {
                _selected_object->edit();
            }
        }
    }

    void Scene::on_imgui_props_map()
    {
        if (ImGui::CollapsingHeader("Background", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Import##opbgp"))
            {
                auto files = open_file_dialog("", "");
                if (!files.empty())
                {
                    setup_background_texture(files[0]);
                }
            }

            ImGui::LabelText("Size", "%d x %d", _grid_size.x, _grid_size.y);
            ImGui::LabelText("Size (px)", "%d x %d", _grid_size.x * tile_size, _grid_size.y * tile_size);
        }

        auto minp = get_active_grid_min();
        auto maxp = get_active_grid_max();
        Region<int32_t> active{ minp.x, minp.y, maxp.x, maxp.y };

        if (ImGui::CollapsingHeader("Tiles", 0))
        {
            if (ImGui::BeginChildFrame(-1, { -1, -1 }, ImGuiWindowFlags_HorizontalScrollbar))
            {
                for (auto y = 0; y < _grid_size.y; ++y)
                {
                    for (auto x = 0; x < _grid_size.x; ++x)
                    {
                        const bool act = active.contains({ x, y });
                        if (act)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
                        }
                        ImGui::Button(ImGui::FormatStr("%dx%d", x, y), {48,48});
                        if (act)
                        {
                            ImGui::PopStyleColor();
                        }
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }
            }
            ImGui::EndChildFrame();
        }

    }

    void Scene::on_imgui_menu()
    {
        _mode = Mode::Undefined;

        if (ImGui::BeginTabItem("Map"))
        {
            _mode = Mode::Map;
            ImGui::Checkbox("Show grid", &_debug_draw_grid);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Navmesh"))
        {
            _mode = Mode::Navmesh;
            ImGui::Checkbox("Show navmesh", &_debug_draw_navmesh);
            ImGui::SameLine();

            if (ImGui::RadioButton("Add", _add_point)) {
                _add_point = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Edit", !_add_point)) {
                _add_point = false;
            }

            ImGui::SameLine();
            ImGui::Dummy({ 16,1 });
            ImGui::SameLine();
            if (ImGui::Button(" Delete "))
            {
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Objects"))
        {
            _mode = Mode::Objects;
            ImGui::Checkbox("Show bounding box##shwobj", &_debug_draw_object);
            ImGui::SameLine();
            ImGui::Dummy({16, 1});
            ImGui::EndTabItem();
        }

    }

    void Scene::on_imgui_workspace()
    {
        auto size = get_scene_size();
        if (size.x && size.y)
        {
            Params params;
            ImVec2 visible_size = ImGui::GetContentRegionAvail();
            params.pos = ImGui::GetWindowPos();


            auto cur = ImGui::GetCursorPos();
            params.dc = ImGui::GetWindowDrawList();
            auto mpos = ImGui::GetMousePos();
            params.mouse = { mpos.x - params.pos.x + ImGui::GetScrollX(), mpos.y - params.pos.y + ImGui::GetScrollY() };

            params.dc->AddImage((ImTextureID)&_canvas.get_texture()->texture, { cur.x + params.pos.x, cur.y + params.pos.y },
                { cur.x + params.pos.x + _canvas.get_width(), cur.y + params.pos.y + _canvas.get_height() }, {0, 1}, {1, 0});

            params.scroll = { ImGui::GetScrollX(), ImGui::GetScrollY() };
            params.pos.x -= ImGui::GetScrollX();
            params.pos.y -= ImGui::GetScrollY();

            ImGui::InvisibleButton("Canvas", ImVec2(size.x, size.y));

            activate_grid(
                Recti(ImGui::GetScrollX(), ImGui::GetScrollY(), visible_size.x, visible_size.y));

            if (ImGui::IsItemClicked(0))
            {
                _drag_active = true;
                _drag_begin = params.mouse;
                _drag_delta = {};
            }

            if (_drag_active && ImGui::IsMouseDown(0))
            {
                _drag_delta.x = params.mouse.x - _drag_begin.x;
                _drag_delta.y = params.mouse.y - _drag_begin.y;
                _drag_begin = params.mouse;
            }
            else
            {
                _drag_active = false;
            }

            switch (_mode)
            {
            case Mode::Map:
                on_imgui_workspace_map(params); break;
            case Mode::Navmesh:
                on_imgui_workspace_navmesh(params); break;
            case Mode::Objects:
                on_imgui_workspace_object(params); break;
            }
        }
    }

    void Scene::on_imgui_workspace_navmesh(Params& params)
    {

    }

    void Scene::on_imgui_workspace_object(Params& params)
    {
        _edit_object = nullptr;

        if (ImGui::IsItemClicked(0))
        {
            if (auto* obj = object_find_at(params.mouse, 1024.f))
            {
                object_select(obj);
            }
            else
            {
                object_select(nullptr);
            }
        }

        if (_drag_active && _selected_object)
        {
            _selected_object->move(_drag_delta);
        }



        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
                    "PROTO",
                    ImGuiDragDropFlags_AcceptPeekOnly | ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<SceneObject *>(ImGui::GetDragData()))
                {
                    _edit_object = object;
                    _edit_object->_position = {params.mouse.x, params.mouse.y};
                }
            }
            if (const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload("PROTO",
                                                 ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<SceneObject *>(ImGui::GetDragData()))
                {
                    object->_position = {params.mouse.x, params.mouse.y};
                    msg::Pack doc;
                    auto ar = doc.create();
                    object_serialize(object, ar);
                    auto arr = doc.get();
                    object_insert(object_deserialize(arr));
                }
            }




            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
                    "SPRITE",
                    ImGuiDragDropFlags_AcceptPeekOnly | ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<SceneObject*>(ImGui::GetDragData()))
                {
                    _edit_object = object;
                    _edit_object->_position = {params.mouse.x, params.mouse.y};
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE", ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<SceneObject*>(ImGui::GetDragData()))
                {
                    object->_position = {params.mouse.x, params.mouse.y};
                    msg::Pack doc;
                    auto ar = doc.create();
                    object_serialize(object, ar);
                    auto arr = doc.get();
                    object_insert(object_deserialize(arr));
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void Scene::on_imgui_workspace_map(Params& params)
    {
    }

    int32_t Scene::IsoObject::depth_get()
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

    float Scene::Params::mouse_distance2(ImVec2 pos) const
    {
        auto dx = mouse.x - pos.x;
        auto dy = mouse.y - pos.y;
        return dx * dx + dy * dy;
    }


    bool ProtoSceneObject::edit()
    {
        bool modified = SceneObject::edit();

        return modified;
    }

    void Scene::IsoManager::update(lq::SpatialDatabase &db,const Recti& region, SceneObject* edit)
    {
        _iso_pool_size = 0;
        auto cb = [&](lq::SpatialDatabase::Proxy *item) {
            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 32);

            _iso_pool[_iso_pool_size]._ptr =
                static_cast<SceneObject *>(item);
            ++_iso_pool_size;
        };

        auto add_pool_item = [&](size_t n) {
            _iso[n] = &_iso_pool[n];
            _iso_pool[n]._depth = 0;
            _iso_pool[n]._depth_active = false;
            _iso_pool[n]._back.clear();
            _iso_pool[n]._ptr->get_iso(_iso_pool[n]._bbox,
                                       _iso_pool[n]._origin);
            _iso_pool[n]._ptr->flag_reset(SceneObjectFlag::Marked);
        };


        // Query active region
        db.map_over_all_objects_in_locality({(float)region.x - tile_size,
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
        if (edit)
        {
            _iso_pool[_iso_pool_size]._ptr = edit;
            _iso.emplace_back();
            add_pool_item(_iso_pool_size);
        }

        

        // Topology sort
        if (_iso.size() > 1)
        {
            // Determine depth relationships
            for (size_t i = 0; i < _iso.size(); ++i)
            {
                auto *a = _iso[i];
                for (size_t j = i + 1; j < _iso.size(); ++j)
                {
                    auto *b = _iso[j];
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
            for (auto *obj : _iso)
                obj->depth_get();

            // Sort objects by depth
            std::sort(_iso.begin(), _iso.end(), [](const IsoObject *a, const IsoObject *b) {
                return a->_depth < b->_depth;
            });
        }
    }

    } // namespace fin
