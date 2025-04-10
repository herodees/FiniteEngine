#include "scene.hpp"
#include "atlas_scene_object.hpp"

#include "utils/dialog_utils.hpp"
#include "utils/imgui_utils.hpp"

namespace fin
{
    constexpr int32_t tile_size(512);

    // Function to generate the scene boundary and hole vertices
    void generateMapWithHole(std::vector<CDT::V2d<float>>& vertices, std::vector<CDT::Edge>& edges, float map_size, float hole_size) {
        // Map boundary (4 corners of the large rectangle)
        vertices.push_back(CDT::V2d<float>(0, 0)); // Bottom-left
        vertices.push_back(CDT::V2d<float>(map_size, 0)); // Bottom-right
        vertices.push_back(CDT::V2d<float>(map_size, map_size)); // Top-right
        vertices.push_back(CDT::V2d<float>(0, map_size)); // Top-left

        // Add edges for the boundary of the scene (outer rectangle)
        edges.push_back(CDT::Edge(0, 1)); // Bottom edge
        edges.push_back(CDT::Edge(1, 2)); // Right edge
        edges.push_back(CDT::Edge(2, 3)); // Top edge
        edges.push_back(CDT::Edge(3, 0)); // Left edge

        // Hole dimensions and position (centered in the scene)
        float hole_left = (map_size - hole_size) / 2;
        float hole_right = hole_left + hole_size;
        float hole_bottom = (map_size - hole_size) / 2;
        float hole_top = hole_bottom + hole_size;

        // Hole boundary vertices (4 corners)
        int hole_offset = vertices.size();
        vertices.push_back(CDT::V2d<float>(hole_left, hole_bottom)); // Bottom-left of hole
        vertices.push_back(CDT::V2d<float>(hole_right, hole_bottom)); // Bottom-right of hole
        vertices.push_back(CDT::V2d<float>(hole_right, hole_top)); // Top-right of hole
        vertices.push_back(CDT::V2d<float>(hole_left, hole_top)); // Top-left of hole

        // Hole edges (to be inserted as constraints)
        edges.push_back(CDT::Edge(hole_offset, hole_offset + 1)); // Bottom edge of hole
        edges.push_back(CDT::Edge(hole_offset + 1, hole_offset + 2)); // Right edge of hole
        edges.push_back(CDT::Edge(hole_offset + 2, hole_offset + 3)); // Top edge of hole
        edges.push_back(CDT::Edge(hole_offset + 3, hole_offset)); // Left edge of hole
    }



    Scene::Scene()
    {
    }

    bool Scene::build_graph()
    {
        decltype(_cdt.vertices) vec;
        vec.swap(_cdt.vertices);

        _cdt = CDT::Triangulation<float>();
        //   _cdt.insertVertices(vec);
          // _cdt.eraseSuperTriangle();
        //   _cdt.eraseOuterTrianglesAndHoles();


        std::vector<CDT::V2d<float>> vertices;
        std::vector<CDT::Edge> edges;
        generateMapWithHole(vertices, edges, 600, 100);
        _cdt.insertVertices(vertices);
        _cdt.insertEdges(edges);
        _cdt.eraseOuterTrianglesAndHoles();
        return true;
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

    bool Scene::setup_navmesh()
    {
        if (_navmesh_points.size() < 4)
            return true;

        _cdt = CDT::Triangulation<float>();
        _cdt.insertVertices(_navmesh_points.begin(), _navmesh_points.end(), getX_V2d, getY_V2d);

        std::vector<CDT::Edge> edges;
        edges.push_back({ 0,1 });
        edges.push_back({ 1,2 });
        edges.push_back({ 2,3 });
        edges.push_back({ 3,0 });
        _cdt.insertEdges(edges);

        _cdt.eraseOuterTrianglesAndHoles();
        //  _cdt.eraseSuperTriangle();
        return true;
    }

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

    void Scene::object_move(SceneObject* obj, float dx, float dy)
    {
        obj->_position.x += dx;
        obj->_position.y += dy;
        _spatial_db.update_for_new_location(obj);
    }

    void Scene::object_moveto(SceneObject* obj, float x, float y)
    {
        obj->_position.x = x;
        obj->_position.y = y;
        _spatial_db.update_for_new_location(obj);
    }

    void Scene::object_move(SceneObject* obj, const Vec2f& d)
    {
        object_move(obj, d.x, d.y);
    }

    void Scene::object_moveto(SceneObject* obj, const Vec2f& p)
    {
        object_moveto(obj, p.x, p.y);
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
        for (auto *obj : _iso)
        {
            if (!obj->_ptr->is_hidden())
                obj->_ptr->render(dc);
        }


        if (_debug_draw_object)
        {
            dc.set_color({255, 0, 0, 255});
            for (auto *obj : _iso)
            {
                dc.render_line(obj->_origin.point1, obj->_origin.point2);
                dc.render_debug_text({obj->_origin.point1.x + 16, obj->_origin.point1.y + 16},
                                     "%d",
                                     obj->_depth);
            }
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
        _iso_pool_size = 0;
        auto cb = [&](lq::SpatialDatabase::Proxy *item) {
            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 32);

            _iso_pool[_iso_pool_size]._ptr = static_cast<SceneObject *>(item);
            ++_iso_pool_size;
        };

        auto add_pool_item = [&](size_t n) {
            _iso[n] = &_iso_pool[n];
            _iso_pool[n]._depth = 0;
            _iso_pool[n]._depth_active = false;
            _iso_pool[n]._back.clear();
            _iso_pool[n]._ptr->get_iso(_iso_pool[n]._bbox, _iso_pool[n]._origin);
            _iso_pool[n]._ptr->flag_reset(SceneObjectFlag::Marked);
        };

        // Query active region
        _spatial_db.map_over_all_objects_in_locality({(float)_active_region.x - tile_size,
                                                      (float)_active_region.y - tile_size,
                                                      (float)_active_region.width + 2*tile_size,
                                                      (float)_active_region.height + 2*tile_size},
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
        if (_edit_object)
        {
            _iso_pool[_iso_pool_size]._ptr = _edit_object;
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
            std::sort(_iso.begin(), _iso.end(), [](const IsoObject *a, const IsoObject *b) { return a->_depth < b->_depth; });
        }
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
        case edit_mode::map:
            on_imgui_props_map();
            break;
        case edit_mode::navmesh:
            on_imgui_props_navmesh();
            break;
        case edit_mode::objects:
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
                if ((size_t)_active_point < _navmesh_points.size())
                {
                    _navmesh_points[_active_point] = _navmesh_points.back();
                    _navmesh_points.pop_back();
                }
            }

            if (ImGui::BeginChildFrame(-1, { -1, -1 }, 0))
            {
                ImGuiListClipper clipper;
                clipper.Begin(_navmesh_points.size());
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        auto& el = _navmesh_points[n];
                        if (ImGui::Selectable(ImGui::FormatStr("%f x %f", el.x, el.y), _active_point == n))
                        {
                            _active_point = n;
                        }
                    }
                }

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
                        if (ImGui::Selectable(ImGui::FormatStr("%p", el)))
                        {

                        }
                    }
                }
            }
            ImGui::EndChildFrame();
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
        _mode = edit_mode::undefined;

        if (ImGui::BeginTabItem("Map"))
        {
            _mode = edit_mode::map;
            ImGui::Checkbox("Show grid", &_debug_draw_grid);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Navmesh"))
        {
            _mode = edit_mode::navmesh;
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
            if (ImGui::Button(" Generate "))
            {
                setup_navmesh();
            }
            ImGui::SameLine();
            ImGui::Dummy({ 16,1 });
            ImGui::SameLine();
            if (ImGui::Button(" Delete "))
            {
                if ((size_t)_active_point < _navmesh_points.size())
                {
                    _navmesh_points[_active_point] = _navmesh_points.back();
                    _navmesh_points.pop_back();
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Objects"))
        {
            _mode = edit_mode::objects;
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

            activate_grid(Recti(ImGui::GetScrollX(), ImGui::GetScrollY(), visible_size.x, visible_size.y));

            switch (_mode)
            {
            case edit_mode::map:
                on_imgui_workspace_map(params); break;
            case edit_mode::navmesh:
                on_imgui_workspace_navmesh(params); break;
            case edit_mode::objects:
                on_imgui_workspace_object(params); break;
            }
        }
    }

    void Scene::on_imgui_workspace_navmesh(Params& params)
    {

        int32_t hover_point = -1;
        for (int32_t n = 0; n < (int32_t)_navmesh_points.size(); ++n)
        {
            auto el = _navmesh_points[n];

            params.dc->AddCircle({ params.pos.x + el.x, params.pos.y + el.y }, 5, n == _active_point ? 0xff00ffff : 0xffffffff, 6);

            if (el.distance_squared({ params.mouse.x, params.mouse.y }) < 5 * 5)
            {
                hover_point = n;
            }
        }

        if (ImGui::IsItemClicked(0))
        {
            if (_add_point) {
                _navmesh_points.push_back({ params.mouse.x, params.mouse.y });
            }
            else {
                _active_point = hover_point;
                _move_point = true;
            }
        }

        if (ImGui::IsMouseDown(0))
        {
            if ((size_t)_active_point < _navmesh_points.size())
            {
                auto el = _navmesh_points[_active_point];
                auto drag = ImGui::GetMouseDragDelta(0);
                params.dc->AddCircle({ drag.x + params.pos.x + el.x, drag.y + params.pos.y + el.y }, 5, 0xff00ffff, 6);
            }
        }
        else
        {
            if (_move_point && (size_t)_active_point < _navmesh_points.size())
            {
                auto drag = ImGui::GetMouseDragDelta(0);
                _navmesh_points[_active_point] += drag;
            }
            _move_point = false;
        }
    }

    void Scene::on_imgui_workspace_object(Params& params)
    {
        _edit_object = nullptr;
        if (ImGui::BeginDragDropTarget())
        {
            ImGuiDragDropFlags drop_target_flags = ImGuiDragDropFlags_AcceptPeekOnly | ImGuiDragDropFlags_AcceptNoPreviewTooltip;
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE", drop_target_flags))
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

    Vec2f Scene::GetCentroid(const CDT::Triangle *tri) const
    {
        auto a = _cdt.vertices[tri->vertices[0]];
        auto b = _cdt.vertices[tri->vertices[1]];
        auto c = _cdt.vertices[tri->vertices[2]];
        return { (a.x + b.x + c.x) / 3.0f,
        (a.y + b.y + c.y) / 3.0f };
    }

    bool Scene::Contains(const Vec2f& point, const CDT::Triangle& tri) const
    {
        auto a = _cdt.vertices[tri.vertices[0]];
        auto b = _cdt.vertices[tri.vertices[2]];
        auto c = _cdt.vertices[tri.vertices[1]];

        auto as_x = point.x - a.x;
        auto as_y = point.y - a.y;

        bool s_ab = (b.x - a.x) * as_y - (b.y - a.y) * as_x > 0;

        if ((c.x - a.x) * as_y - (c.y - a.y) * as_x > 0 == s_ab)
            return false;
        if ((c.x - b.x) * (point.y - b.y) - (c.y - b.y) * (point.x - b.x) > 0 != s_ab)
            return false;
        return true;
    }


    const CDT::Triangle* Scene::FindTriangle(const Vec2f& point)
    {
        for (auto& tri : _cdt.triangles) {
            if (Contains(point, tri)) {
                return &tri;
            }
        }
        return nullptr;
    }

    void Scene::AddPoint(Vec2f p)
    {
        CDT::V2d<float> pos{ p.x, p.y };

        if (std::find(_cdt.vertices.begin(), _cdt.vertices.end(), pos) == _cdt.vertices.end())
        {
            decltype(_cdt.vertices) vec;
            vec.swap(_cdt.vertices);
            vec.push_back(pos);

            _cdt = CDT::Triangulation<float>();
            _cdt.insertVertices(vec);
            _cdt.eraseSuperTriangle();

            build_graph();
        }
    }

    std::vector<const CDT::Triangle*> Scene::FindPath(Vec2f start, Vec2f goal)
    {
        struct PathNode
        {
            const CDT::Triangle* triangle;
            float g_cost, h_cost;
            PathNode* parent;

            float FCost() const { return g_cost + h_cost; }
        };

        struct CompareFCost {
            bool operator()(const PathNode* a, const PathNode* b) const {
                return a->FCost() > b->FCost();
            }
        };

        const CDT::Triangle* startTri = FindTriangle(start);
        const CDT::Triangle* goalTri = FindTriangle(goal);
        if (!startTri || !goalTri) return {};

        std::priority_queue<PathNode*, std::vector<PathNode*>, CompareFCost> openSet;
        std::unordered_map<const CDT::Triangle*, PathNode> allNodes;

        allNodes[startTri] = { startTri, 0.0f, GetCentroid(startTri).distance(goal), nullptr };
        openSet.push(&allNodes[startTri]);

        while (!openSet.empty())
        {
            PathNode* current = openSet.top();
            openSet.pop();

            if (current->triangle == goalTri)
            {
                std::vector<const CDT::Triangle*> path;
                while (current)
                {
                    path.push_back(current->triangle);
                    current = current->parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (auto tr : current->triangle->neighbors)
            {
                if (tr == -1)continue;
                CDT::Triangle* neighbor = &_cdt.triangles[tr];

                float gCost = current->g_cost + GetCentroid(current->triangle).distance(GetCentroid(neighbor));
                float hCost = GetCentroid(neighbor).distance(goal);

                if (!allNodes.count(neighbor) || gCost < allNodes[neighbor].g_cost)
                {
                    allNodes[neighbor] = { neighbor, gCost, hCost, current };
                    openSet.push(&allNodes[neighbor]);
                }
            }
        }
        return {};
    }

    bool Scene::FindSharedEdge(const CDT::Triangle& tri1, const CDT::Triangle* tri2, Vec2f& outStart, Vec2f& outEnd)
    {
        if (!tri2) return false; // No neighbor -> no shared edge

        for (int i = 0; i < 3; ++i) {
            auto a = tri1.vertices[i];
            auto b = tri1.vertices[(i + 1) % 3];

            for (int j = 0; j < 3; ++j) {
                auto c = tri2->vertices[j];
                auto d = tri2->vertices[(j + 1) % 3];

                if ((a == c && b == d) || (a == d && b == c)) {
                    outStart = _cdt.vertices[a];
                    outEnd = _cdt.vertices[b];
                    return true;
                }
            }
        }
        return false;
    }


    std::vector<std::pair<Vec2f, Vec2f>>  Scene::ExtractPortals(
        const std::vector<const CDT::Triangle*>& trianglePath,
        Vec2f start, Vec2f goal)
    {
        std::vector<std::pair<Vec2f, Vec2f>> portals;

        // Add a portal from the start point to the first shared edge
        if (trianglePath.size() > 1) {
            Vec2f edgeStart, edgeEnd;
            if (FindSharedEdge(*trianglePath[0], trianglePath[1], edgeStart, edgeEnd)) {
                //     portals.push_back({ start, start });
                portals.push_back({ edgeStart, edgeEnd });
            }
        }

        // Extract shared edges between adjacent triangles
        for (int i = 1; i < (int)trianglePath.size() - 1; ++i) {
            Vec2f edgeStart, edgeEnd;
            if (FindSharedEdge(*trianglePath[i], trianglePath[i + 1], edgeStart, edgeEnd)) {
                portals.push_back({ edgeStart, edgeEnd });
            }
        }

        // Add the goal as the last portal
        portals.push_back({ goal, goal });

        return portals;
    }

    void Scene::EnsureCorrectPortalOrder(std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start) {
        Vec2f prev = start;  // Start position

        for (auto& portal : portals) {
            Vec2f left = portal.first;
            Vec2f right = portal.second;

            Vec2f direction = left - prev;  // Path direction
            Vec2f edge = right - left;      // Portal edge direction

            // Check if the left point is actually on the left
            if (direction.cross(edge) < 0.0f) {
                std::swap(portal.first, portal.second); // Swap left and right if order is wrong
            }

            prev = (left + right) * 0.5f;  // Move to next portal (approximate midpoint)
        }
    }

    std::vector<Vec2f> Scene::ComputeSmoothPath(const std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start)
    {
        std::vector<Vec2f> path;
        path.reserve(portals.size() + 1);
        path.push_back(start);

        Vec2f apex = start;
        Vec2f left = portals[0].first;
        Vec2f right = portals[0].second;
        size_t apexIndex = 0, leftIndex = 0, rightIndex = 0;

        for (size_t i = 1; i < portals.size(); ++i) {
            Vec2f newLeft = portals[i].first;
            Vec2f newRight = portals[i].second;

            // Pre-calculate difference vectors relative to the current apex.
            Vec2f dNewLeft = newLeft - apex;
            Vec2f dLeft = left - apex;
            Vec2f dRight = right - apex;

            // Check if the new left vertex is outside the funnel (i.e., not clearly left of the current right boundary)
            float crossNewLeftRight = dNewLeft.cross(dRight);
            if (crossNewLeftRight >= 0.0f) {
                float crossNewLeftLeft = dNewLeft.cross(dLeft);
                if (crossNewLeftLeft <= 0.0f) {
                    // New left is still to the left of the left boundary: update left.
                    left = newLeft;
                    leftIndex = i;
                }
                else {
                    // Funnel collapse on left: update the apex.
                    apex = left;
                    apexIndex = leftIndex;
                    path.push_back(apex);

                    // Reset the funnel boundaries starting from the new apex.
                    left = apex;
                    right = apex;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    // Reprocess this portal by setting i back.
                    i = apexIndex;
                    continue;
                }
            }

            // Pre-calculate difference vector for new right.
            Vec2f dNewRight = newRight - apex;
            float crossNewRightLeft = dNewRight.cross(dLeft);
            if (crossNewRightLeft <= 0.0f) {
                float crossNewRightRight = dNewRight.cross(dRight);
                if (crossNewRightRight >= 0.0f) {
                    // New right is still to the right of the right boundary: update right.
                    right = newRight;
                    rightIndex = i;
                }
                else {
                    // Funnel collapse on right: update the apex.
                    apex = right;
                    apexIndex = rightIndex;
                    path.push_back(apex);

                    // Reset the funnel boundaries.
                    left = apex;
                    right = apex;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    // Reprocess this portal by setting i back.
                    i = apexIndex;
                    continue;
                }
            }
        }
        // Append the final endpoint from the last portal.
        path.push_back(portals.back().first);
        return path;
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

    } // namespace fin
