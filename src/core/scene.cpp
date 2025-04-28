#include "scene.hpp"
#include "utils/dialog_utils.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{
    constexpr int32_t tile_size(512);

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
    }

    const std::string& Scene::get_path() const
    {
        return _path;
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

    void Scene::activate_grid(const Vec2f& origin)
    {
        auto s = _active_region.size();
        activate_grid(Recti(origin.x - s.x * 0.5f, origin.y - s.y * 0.5f, origin.x + s.x * 0.5f, origin.y + s.y * 0.5f));
    }

    void Scene::set_size(Vec2f size)
    {
        if (_size != size)
        {
            _size = size;
            for (auto* ly : _layers)
            {
                ly->resize(_size);
            }
        }
    }

    int32_t Scene::add_layer(SceneLayer* layer)
    {
        _layers.emplace_back(layer);
        layer->_parent = this;
        layer->resize(_size);
        return int32_t(_layers.size()) - 1;
    }

    void Scene::delete_layer(int32_t n)
    {
        delete _layers[n];
        _layers.erase(_layers.begin() + n);
    }


    Vec2i Scene::get_active_grid_size() const
    {
        return _active_region.size();
    }

    Vec2f Scene::get_active_grid_center() const
    {
        return _active_region.center();
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

    void Scene::object_serialize(BasicSceneObject* obj, msg::Writer& ar)
    {
        ar.begin_object();
        obj->serialize(ar);
        if (obj->is_named())
        {
            ar.member(Sc::Name, obj->_name);
        }
        ar.end_object();
    }

    BasicSceneObject* Scene::object_deserialize(msg::Value& ar)
    {
        auto ot = ar[Sc::Uid].get(0ull);
        if (BasicSceneObject* obj = SceneFactory::instance().create(ot))
        {
            obj->deserialize(ar);
            auto id = ar[Sc::Name].str();
            if (!id.empty())
            {
                name_object(obj, id);
            }
            return obj;
        }
        return nullptr;
    }

    void Scene::object_insert(BasicSceneObject* obj)
    {
        if (!obj)
            return;
        obj->_id = _scene.size();
        _scene.push_back(obj);
        _spatial_db.update_for_new_location(obj);
    }

    void Scene::object_remove(BasicSceneObject* obj)
    {
        if (obj->is_named())
        {
            name_object(obj, {});
        }
        _spatial_db.remove_from_bin(obj);
        const auto id   = obj->_id;
        _scene[id]      = _scene.back();
        _scene[id]->_id = id;
        _scene.pop_back();
    }

    void Scene::object_select(BasicSceneObject* obj)
    {
        if (_edit._selected_object != obj)
        {
            _edit._selected_object = obj;
        }
    }

    void Scene::object_destroy(BasicSceneObject* obj)
    {
        if (obj)
        {
            object_remove(obj);
        }
        delete obj;
    }

    void Scene::object_moveto(BasicSceneObject* obj, Vec2f pos)
    {
        obj->_position = pos;
        _spatial_db.update_for_new_location(obj);
    }

    void Scene::region_serialize(SceneRegion* obj, msg::Writer& ar)
    {
        ar.begin_object();
        obj->serialize(ar);
        if (obj->is_named())
        {
            ar.member(Sc::Name, obj->_name);
        }
        ar.end_object();
    }

    SceneRegion* Scene::region_deserialize(msg::Value& ar)
    {
        if (SceneRegion* obj = new SceneRegion)
        {
            obj->deserialize(ar);
            auto id = ar[Sc::Name].str();
            if (!id.empty())
            {
                name_object(obj, id);
            }
            return obj;
        }
        return nullptr;
    }

    void Scene::region_insert(SceneRegion* obj)
    {
        if (!obj)
            return;
        obj->_id    = _regions.size();
        _regions.push_back(obj);
    }

    void Scene::region_remove(SceneRegion* obj)
    {
        if (obj->is_named())
        {
            name_object(obj, {});
        }
        const auto id   = obj->_id;
        _regions[id]    = _regions.back();
        _regions[id]->_id = id;
        _regions.pop_back();
    }

    void Scene::region_select(SceneRegion* obj)
    {
        if (_edit._selected_region != obj)
        {
            _edit._selected_region = obj;
        }
    }

    void Scene::region_destroy(SceneRegion* obj)
    {
        if (obj)
        {
            region_remove(obj);
        }
        delete obj;
    }

    void Scene::region_moveto(SceneRegion* obj, Vec2f pos)
    {
        if (obj->_region.size() == 0)
            return;

        const Vec2f off = pos - obj->get_point(0);
        for (uint32_t n = 0; n < obj->_region.size(); n += 2)
        {
            obj->_region.set_item(n, obj->_region.get_item(n).get(0.f) + off.x);
            obj->_region.set_item(n + 1, obj->_region.get_item(n + 1).get(0.f) + off.y);
        }
        obj->change();
    }

    void Scene::name_object(ObjectBase* obj, std::string_view name)
    {
        if (!obj)
            return;

        if (obj->is_named())
        {
            auto it = _named_object.find(obj->_name);
            if (it != _named_object.end())
            {
                _named_object.erase(it);
            }
            obj->_name = nullptr;
        }

        if (!name.empty())
        {
            auto [it, inserted] = _named_object.try_emplace(std::string(name), obj);
            if (inserted)
            {
                obj->_name = it->first.c_str();
            }
        }
    }

    ObjectBase* Scene::find_object_by_name(std::string_view name)
    {
        auto it = _named_object.find(name);
        if (it == _named_object.end())
            return nullptr;
        return it->second;
    }

    SpriteSceneObject* Scene::object_find_at(Vec2f position, float radius)
    {
        struct
        {
            lq::SpatialDatabase::Proxy *obj = nullptr;
            float dist = FLT_MAX;
        } out;

        auto cb = [&out, position](lq::SpatialDatabase::Proxy *obj, float dist) {
            auto *object = static_cast<SpriteSceneObject *>(obj);
            Region<float> bbox;
            Line<float> line;
            if (object->bounding_box().contains(position))
            {
                out.obj = obj;
            }
        };

        _spatial_db.map_over_all_objects_in_locality(position.x, position.y, radius, cb);
        return static_cast<SpriteSceneObject *>(out.obj);
    }

    SceneRegion* Scene::region_find_at(Vec2f position)
    {
        for (auto* reg : _regions)
        {
            if (reg->contains(position))
            {
                return reg;
            }
        }
        return nullptr;
    }

    void Scene::render(Renderer& dc)
    {
        if (!_canvas)
            return;

        dc.set_debug(_mode == Mode::Objects);

        // Draw on the canvas
        BeginTextureMode(*_canvas.get_texture());
        ClearBackground({0, 0, 0, 255});

        auto minpos = get_active_grid_min();
        auto maxpos = get_active_grid_max();

        dc.set_origin({(float)_active_region.x, (float)_active_region.y});

        BeginMode2D(dc._camera);

        for (auto* el : _layers)
        {
            el->render(dc);
        }


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

        dc.set_color(WHITE);
        for (auto* obj : _iso_manager._static)
        {
            if (!obj->is_hidden())
                obj->render(dc);
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

        if (_edit._selected_object)
        {
            dc.set_color(WHITE);
            _edit._selected_object->edit_render(dc);
        }

        if (_debug_draw_navmesh)
        {
            dc.set_color({255, 255, 0, 255});
            for (auto* obj : _iso_manager._iso)
            {
                auto& coll = obj->_ptr->collision();
                auto  sze  = coll.size();
                if (coll.is_array() && sze >= 4)
                {
                    auto p = obj->_ptr->position();
                    for (uint32_t n = 0; n < sze; n += 2)
                    {
                        Vec2f from(coll.get_item(n % sze).get(0.f), coll.get_item((n + 1) % sze).get(0.f));
                        Vec2f to(coll.get_item((n + 2) % sze).get(0.f), coll.get_item((n + 3) % sze).get(0.f));
                        dc.render_line(from + p, to + p);
                    }
                }
            }

            dc.set_color({128, 128, 128, 255});
            auto edges = _pathfinder.GetEdgesForDebug();
            for (auto& el : edges)
            {
                dc.render_line(el.b, el.e);
            }
        }

        if (_debug_draw_regions)
        {
            dc.set_color(WHITE);
            for (auto* reg : _regions)
            {
                reg->edit_render(dc);
            }

            if (_edit._selected_region)
            {
                dc.set_color({255, 255, 0, 255});
                auto bbox = _edit._selected_region->bounding_box();
                dc.render_line_rect(bbox.rect());

                for (auto n = 0; n < _edit._selected_region->get_size(); ++n)
                {
                    auto pt = _edit._selected_region->get_point(n);
                    if (_edit._active_point == n)
                        dc.render_circle(pt, 3);
                    else
                        dc.render_line_circle(pt, 3);
                }
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

        dc.set_debug(false);
    }

    void Scene::update(float dt)
    {
        for (auto* el : _layers)
        {
            el->update(dt);
        }




        _iso_manager.update(_spatial_db, _active_region, _edit._edit_object);

        if (_mode == Mode::Running)
        {
            for (auto* obj : _iso_manager._static)
            {
                if (!obj->is_disabled())
                    obj->update(dt);
            }
            for (auto* obj : _iso_manager._iso)
            {
                if (!obj->_ptr->is_disabled())
                    obj->_ptr->update(dt);
            }
        }
    }



    void Scene::clear()
    {
        std::for_each(_scene.begin(), _scene.end(), [](auto* p) { delete p; });
        std::for_each(_layers.begin(), _layers.end(), [](auto* p) { delete p; });
        _grid_size = {};
        _grid_active.clear();
        _grid_texture.clear();
        _scene.clear();
        _spatial_db.init({}, 0, 0);
        _grid_surface.clear();
        _grid_texture.clear();
        _layers.clear();
    }

    void Scene::generate_navmesh()
    {
        std::vector<NavMesh::Polygon> polygons;
        std::for_each(_scene.begin(),
                      _scene.end(),
                      [&polygons](BasicSceneObject* obj)
                      {
                          if (obj->isometric_sort())
                          {
                              auto coll = static_cast<IsoSceneObject*>(obj)->collision();
                              if (coll.size() >= 6)
                              {
                                  auto  pos  = obj->position();
                                  auto& poly = polygons.emplace_back();
                                  for (uint32_t n = 0; n < coll.size(); n += 2)
                                  {
                                      poly.AddPoint(pos.x + coll.get_item(n).get(0.f),
                                                    pos.y + coll.get_item(n + 1).get(0.f));
                                  }
                              }
                          }
                      });
        _pathfinder.AddPolygons(polygons, 16);
    }

    RenderTexture2D& Scene::canvas()
    {
        return _canvas;
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

            ar.key("regions");
            ar.begin_array();
            {
                for (auto* obj : _regions)
                {
                    region_serialize(obj, ar);
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

        auto rbs = ar["regions"].elements();
        for (auto el : rbs)
        {
            region_insert(region_deserialize(el));
        }

        _grid_texture.resize(_grid_surface.size());
    }

    void Scene::load(std::string_view path)
    {
        _path = path;
        msg::Pack pack;
        int       size{};
        auto*     txt = LoadFileData(_path.c_str(), &size);
        pack.data().assign(txt, txt + size);
        UnloadFileData(txt);
        auto ar = pack.get();
        deserialize(ar);
    }

    void Scene::save(std::string_view path)
    {
        _path = path;
        msg::Pack pack;
        serialize(pack);
        auto ar = pack.data();
        SaveFileData(_path.c_str(), pack.data().data(), pack.data().size());
    }

    void Scene::on_imgui_explorer()
    {
        SceneFactory::instance().show_explorer();
    }

    void Scene::on_imgui_props()
    {
        if (!ImGui::Begin("Properties"))
        {
            ImGui::End();
            return;
        }

        switch (_mode)
        {
            case Mode::Map:
                on_imgui_props_map();
                break;
            case Mode::Objects:
                on_imgui_props_object();
                break;
            case Mode::Prefab:
                SceneFactory::instance().show_properties();
                break;
        }

        ImGui::End();
    }

    void Scene::on_imgui_props_object()
    {
        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginChildFrame(-2, {-1, 100}, 0))
            {
                int n = 0;
                for (auto* ly : _layers)
                {
                    ImGui::PushID(n);

                    ImGui::SetNextItemAllowOverlap();
                    if (ImGui::Selectable(" ", _edit._active_layer == n))
                        _edit._active_layer = n;

                    ImGui::SameLine();
                    if (ImGui::SmallButton(ly->is_hidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE))
                    {
                        ly->hide(!ly->is_hidden());
                    }

                    ImGui::SameLine();
                    ImGui::Text("%s %s", ly->icon().data(), ly->name().c_str());

                    ImGui::PopID();
                    ++n;
                }
            }
            ImGui::EndChildFrame();
        }

        if (ImGui::CollapsingHeader("Items", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (size_t(_edit._active_layer) < _layers.size())
            {
                ImGui::PushID("lyit");
                _layers[_edit._active_layer]->imgui_update(true);
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (size_t(_edit._active_layer) < _layers.size())
            {
                ImGui::PushID("lypt");
                _layers[_edit._active_layer]->imgui_update(false);
                ImGui::PopID();
            }
        }
    }

    void Scene::on_imgui_props_region()
    {
        if (ImGui::CollapsingHeader("Regions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button(" " ICON_FA_BAN " "))
            {
                if (_edit._selected_region)
                {
                    region_destroy(_edit._selected_region);
                    _edit._selected_region = nullptr;
                }
            }
            if (ImGui::BeginChildFrame(-1, {-1, 250}, 0))
            {
                ImGuiListClipper clipper;
                clipper.Begin(_regions.size());
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        ImGui::PushID(n);
                        auto* el = _regions[n];
                        const char* name = el->_name;
                        if (!name)
                            name = ImGui::FormatStr("Region %p", el);

                        if (ImGui::Selectable(name, el == _edit._selected_region))
                        {
                            region_select(el);
                        }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChildFrame();
        }

        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (_edit._selected_region)
            {
                _edit._buffer = _edit._selected_region->is_named() ? _edit._selected_region->_name : "";
                if (ImGui::InputText("Name", &_edit._buffer))
                {
                    name_object(_edit._selected_region, _edit._buffer);
                }

                _edit._selected_region->imgui_update();
            }
        }
    }

    void Scene::on_imgui_props_map()
    {
        static int s_val[2]{};

        if (ImGui::CollapsingHeader("Size", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Resize..."))
            {
                ImGui::OpenPopup("Resize");
                s_val[0] = _size.x;
                s_val[1] = _size.y;
            }
            if (ImGui::BeginPopupModal("Resize", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Set new scene size!");
                ImGui::InputInt2("Size", s_val);

                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    set_size(Vec2f(s_val[0], s_val[1]));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::LabelText("Size (px)", "%.0f x %.0f", _size.x, _size.y);
        }
        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button(" " ICON_FA_LAPTOP " Add "))
                ImGui::OpenPopup("LayerMenu");

            if (ImGui::BeginPopup("LayerMenu"))
            {
                if (ImGui::MenuItem(ICON_FA_MAP_PIN " Isometric layer"))
                    add_layer(SceneLayer::create(SceneLayer::Type::Isometric));
                if (ImGui::MenuItem(ICON_FA_IMAGE " Sprite layer"))
                    add_layer(SceneLayer::create(SceneLayer::Type::Sprite));
                if (ImGui::MenuItem(ICON_FA_MAP_LOCATION_DOT " Region layer"))
                    add_layer(SceneLayer::create(SceneLayer::Type::Region));
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(" " ICON_FA_ARROW_UP " "))
            {
                _edit._active_layer = move_layer(_edit._active_layer, false);
            }
            ImGui::SameLine();
            if (ImGui::Button(" " ICON_FA_ARROW_DOWN " "))
            {
                _edit._active_layer = move_layer(_edit._active_layer, true);
            }
            ImGui::SameLine();
            ImGui::InvisibleButton("##ib", {-34, 1});
            ImGui::SameLine();
            if (ImGui::Button(" " ICON_FA_BAN " "))
            {
                if (size_t(_edit._active_layer) < _layers.size())
                {
                    delete_layer(_edit._active_layer);
                }
            }

            if (ImGui::BeginChildFrame(-2, {-1, 100}))
            {
                int n = 0;
                for (auto* ly : _layers)
                {
                    ImGui::PushID(n);
                    if (ImGui::Selectable(ImGui::FormatStr("%s %s", ly->icon().data(), ly->name().c_str()),
                                          _edit._active_layer == n))
                        _edit._active_layer = n;
                    ImGui::PopID();
                    ++n;
                }
            }
            ImGui::EndChildFrame();
            if (size_t(_edit._active_layer) < _layers.size())
            {
                ImGui::InputText("Name", &_layers[_edit._active_layer]->name());
            }
        }

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
        _debug_draw_regions = false;

        if (ImGui::BeginTabItem(ICON_FA_GEAR " Setup"))
        {
            _mode = Mode::Map;
            ImGui::Checkbox("Show grid", &_debug_draw_grid);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(ICON_FA_BRUSH " Edit"))
        {
            _mode = Mode::Objects;
            ImGui::Checkbox("Show bounding box##shwobj", &_debug_draw_object);
            ImGui::SameLine();
            ImGui::Checkbox("Show navmesh", &_debug_draw_navmesh);
            ImGui::SameLine();
            if (ImGui::Button(" Generate "))
            {
                generate_navmesh();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(ICON_FA_BOX_ARCHIVE " Prefabs"))
        {
            _mode = Mode::Prefab;
            SceneFactory::instance().show_menu();
            ImGui::EndTabItem();
        }
    }

    void Scene::on_imgui_workspace()
    {
        if (_mode == Mode::Prefab)
        {
            return SceneFactory::instance().show_workspace();
        }
        else
        {
            SceneFactory::instance().center_view();
        }

        auto size = get_scene_size();
        if (size.x && size.y)
        {
            Params params;
            ImVec2 visible_size = ImGui::GetContentRegionAvail();
            params.pos = ImGui::GetWindowPos();


            auto cur = ImGui::GetCursorPos();
            params.dc = ImGui::GetWindowDrawList();
            auto mpos = ImGui::GetMousePos();
            params.mouse = {mpos.x - params.pos.x - cur.x + ImGui::GetScrollX(),
                            mpos.y - params.pos.y - cur.y + ImGui::GetScrollY()};

            params.dc->AddImage((ImTextureID)&_canvas.get_texture()->texture, { cur.x + params.pos.x, cur.y + params.pos.y },
                { cur.x + params.pos.x + _canvas.get_width(), cur.y + params.pos.y + _canvas.get_height() }, {0, 1}, {1, 0});

            params.scroll = { ImGui::GetScrollX(), ImGui::GetScrollY() };
            params.pos.x -= ImGui::GetScrollX();
            params.pos.y -= ImGui::GetScrollY();

            ImGui::InvisibleButton("Canvas", ImVec2(size.x, size.y));

            activate_grid(
                Recti(ImGui::GetScrollX(), ImGui::GetScrollY(), visible_size.x, visible_size.y));

            _drag.update(params.mouse.x, params.mouse.y);

            switch (_mode)
            {
                case Mode::Map:
                    on_imgui_workspace_map(params);
                    break;
                case Mode::Objects:
                    on_imgui_workspace_object(params);
                    break;
            }
        }
    }

    void Scene::on_imgui_workspace_object(Params& params)
    {
        _edit._edit_object = nullptr;

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
        if (ImGui::IsItemClicked(1))
        {
            object_select(nullptr);
        }

        if (_drag._active && _edit._selected_object)
        {
            object_moveto(_edit._selected_object, _drag._delta + _edit._selected_object->position());
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB",
                                                                           ImGuiDragDropFlags_AcceptPeekOnly |
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<BasicSceneObject*>(ImGui::GetDragData("PREFAB")))
                {
                    _edit._edit_object      = object;
                    _edit._edit_object->_position = {params.mouse.x, params.mouse.y};
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB", ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                if (auto object = static_cast<BasicSceneObject*>(ImGui::GetDragData("PREFAB")))
                {
                    object->_position = {params.mouse.x, params.mouse.y};
                    msg::Pack doc;
                    auto      ar = doc.create();
                    object_serialize(object, ar);
                    auto arr = doc.get();
                    object_insert(object_deserialize(arr));
                }
            }

            ImGui::EndDragDropTarget();
        }
    }

    void Scene::on_imgui_workspace_region(Params& params)
    {
        // Edit region points
        if (_edit_region)
        {
            if (_edit._selected_region)
            {
                if (ImGui::IsItemClicked(0))
                {
                    _edit._active_point = _edit._selected_region->find_point(params.mouse, 5);
                }
                if (ImGui::IsItemClicked(1))
                {
                    region_select(nullptr);
                }
            }
            else if (ImGui::IsItemClicked(0))
            {
                _edit._selected_region = region_find_at(params.mouse);
            }

            if (_drag._active && _edit._active_point != -1 && _edit._selected_region)
            {
                auto pt = _edit._selected_region->get_point(_edit._active_point) + _drag._delta;
                _edit._selected_region->set_point(pt, _edit._active_point);
            }
        }
        // Create region
        else
        {
            if (ImGui::IsItemClicked(0))
            {
                if (_edit._selected_region)
                {
                    _edit._selected_region->insert_point(params.mouse, _edit._active_point + 1);
                    _edit._active_point = _edit._active_point + 1;
                }
                else
                {
                    region_select(new SceneRegion);
                    region_insert(_edit._selected_region);
                    _edit._active_point = 0;
                    _edit._selected_region->insert_point(params.mouse);
                }
            }
            if (ImGui::IsItemClicked(1))
            {
                _edit._active_point = -1;
                region_select(nullptr);
            }
        }
    }

    void Scene::on_imgui_workspace_map(Params& params)
    {
    }

    int32_t Scene::move_layer(int32_t layer, bool up)
    {
        if (up)
        {
            if (size_t(layer + 1) < _layers.size() && size_t(layer) < _layers.size())
            {
                std::swap(_layers[layer], _layers[layer + 1]);
                return layer + 1;
            }
        }
        else
        {
            if (size_t(layer - 1) < _layers.size() && size_t(layer) < _layers.size())
            {
                std::swap(_layers[layer], _layers[layer - 1]);
                return layer - 1;
            }
        }
        return layer;
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



    void Scene::IsoManager::update(lq::SpatialDatabase& db, const Recti& region, BasicSceneObject* edit)
    {
        _iso_pool_size = 0;
        _static.clear();

        auto cb = [&](lq::SpatialDatabase::Proxy* item)
        {
            if (!static_cast<BasicSceneObject*>(item)->isometric_sort())
            {
                _static.push_back(static_cast<BasicSceneObject*>(item));
            }
            else
            {
                if (_iso_pool_size >= _iso_pool.size())
                    _iso_pool.resize(_iso_pool_size + 32);

                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(item);
                ++_iso_pool_size;
            }
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
            if (edit->isometric_sort())
            {
                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(edit);
                _iso.emplace_back();
                add_pool_item(_iso_pool_size);
            }
            else
            {
                _static.push_back(edit);
            }
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

    void DragData::update(float x, float y)
    {
        if (ImGui::IsItemClicked(0))
        {
            _active = true;
            _begin  = {x, y};
            _delta  = {};
        }

        if (_active && ImGui::IsMouseDown(0))
        {
            _delta.x = x - _begin.x;
            _delta.y = y - _begin.y;
            _begin   = {x, y};
        }
        else
        {
            _active = false;
        }
    }



} // namespace fin
