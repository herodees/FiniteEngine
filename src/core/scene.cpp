#include "scene.hpp"
#include "utils/dialog_utils.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{
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

    void Scene::activate_grid(const Recti& screen)
    {
        _active_region = screen;

        auto canvas_size = _canvas.get_size();
        auto new_canvas_size = _active_region.size();

        if (canvas_size != new_canvas_size)
        {
            _canvas.create(new_canvas_size.x, new_canvas_size.y);
        }

        for (auto* ly : _layers)
        {
            ly->activate(_active_region);
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

    Vec2i Scene::get_scene_size() const
    {
        return _size;
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
        ClearBackground(_background);

        dc.set_origin({(float)_active_region.x, (float)_active_region.y});

        BeginMode2D(dc._camera);

        for (auto* el : _layers)
        {
            el->render(dc);
        }

        if (auto* lyr = active_layer())
        {
            lyr->render_edit(dc);
        }

        /*
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
        */

        EndMode2D();
        EndTextureMode();

        dc.set_debug(false);
    }

    void Scene::update(float dt)
    {
        if (_mode == Mode::Running)
        {
            for (auto* el : _layers)
            {
                el->update(dt);
            }
        }
    }

    void Scene::clear()
    {
        std::for_each(_layers.begin(), _layers.end(), [](auto* p) { delete p; });
        _layers.clear();
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
            ar.member("width", _size.x);
            ar.member("height", _size.y);

            ar.key("background");
            ar.begin_array()
                .value((int)_background.r)
                .value((int)_background.g)
                .value((int)_background.b)
                .value((int)_background.a)
                .end_array();

            ar.key("layers");
            ar.begin_array();
            for (auto* ly : _layers)
            {
                ar.begin_object();
                ly->serialize(ar);
                ar.end_object();
            }
            ar.end_array();
        }
        ar.end_object();
    }

    void Scene::deserialize(msg::Value& ar)
    {
        clear();

        if (ar.is_object())
        {
            _size.x = ar["width"].get(0.f);
            _size.y = ar["height"].get(0.f);

            auto bg       = ar["background"];
            _background.r = bg[0].get(0);
            _background.g = bg[1].get(0);
            _background.b = bg[2].get(0);
            _background.a = bg[3].get(0);

            auto layers = ar["layers"];
            for (auto ly : layers.elements())
            {
                if (auto* l = SceneLayer::create(ly, this))
                {
                    add_layer(l);
                }
            }
        }
        else
        {
            //TODO: Trace error
        }
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

    void Scene::imgui_explorer()
    {
        SceneFactory::instance().imgui_explorer();
    }

    void Scene::imgui_props()
    {
        if (!ImGui::Begin("Properties"))
        {
            ImGui::End();
            return;
        }

        switch (_mode)
        {
            case Mode::Setup:
                imgui_props_setup();
                break;
            case Mode::Objects:
                imgui_props_object();
                break;
            case Mode::Prefab:
                SceneFactory::instance().imgui_properties();
                break;
        }

        ImGui::End();
    }

    void Scene::imgui_props_object()
    {
        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
        {

            if (ImGui::BeginChildFrame(ImGui::GetID("lyrssl"), {-1, 100}, 0))
            {
                int n = 0;
                for (auto* ly : _layers)
                {
                    if (ImGui::LineSelect(ImGui::GetID(ly), _edit._active_layer == n)
                            .Space()
                            .PushStyle(ImStyle_Button, 1)
                            .Text(ly->is_hidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE)
                            .PopStyle()
                            .Space()
                            .Text(ly->icon())
                            .Space()
                            .Text(ly->name())
                            .Spring()
                            .Text(_edit._active_layer == n ? ICON_FA_BRUSH : "")
                            .End())
                    {
                        _edit._active_layer = n;
                        if (ImGui::Line().HoverId() == 1)
                        {
                            ly->hide(!ly->is_hidden());
                        }
                    }
                    ++n;
                }
            }
            ImGui::EndChildFrame();
        }

        if (ImGui::CollapsingHeader("Items", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (auto* lyr = active_layer())
            {
                ImGui::PushID("lyit");
                lyr->imgui_update(true);
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (auto* lyr = active_layer())
            {
                ImGui::PushID("lypt");
                lyr->imgui_update(false);
                ImGui::PopID();
            }
        }
    }

    void Scene::imgui_props_setup()
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
            if (ImGui::LineItem(-5, {-1, ImGui::GetFrameHeight()})
                    .PushStyle(ImStyle_Button, 1)
                    .Text(ICON_FA_LAPTOP " Add ")
                    .PopStyle()
                    .PushStyle(ImStyle_Button, 2)
                    .Text(ICON_FA_ARROW_UP)
                    .PopStyle()
                    .PushStyle(ImStyle_Button, 3)
                    .Text(ICON_FA_ARROW_DOWN)
                    .PopStyle()
                    .Spring()
                    .PushStyle(ImStyle_Button, 4)
                    .Text(ICON_FA_BAN)
                    .PopStyle()
                    .End())
            {
                if (ImGui::Line().HoverId() == 1)
                {
                    ImGui::OpenPopup("LayerMenu");
                }

                if (ImGui::Line().HoverId() == 2)
                {
                    _edit._active_layer = move_layer(_edit._active_layer, false);
                }
                if (ImGui::Line().HoverId() == 3)
                {
                    _edit._active_layer = move_layer(_edit._active_layer, true);
                }
                if (ImGui::Line().HoverId() == 4 && active_layer())
                {
                    delete_layer(_edit._active_layer);
                }
            }

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

            if (ImGui::BeginChildFrame(-2, {-1, 100}))
            {
                int n = 0;
                for (auto* ly : _layers)
                {
                    if (ImGui::LineSelect(ImGui::GetID(ly), _edit._active_layer == n)
                            .Text(ly->icon())
                            .Space()
                            .Text(ly->name())
                            .End())
                    {
                        _edit._active_layer = n;
                    }
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
            float clr[4] = {_background.r / 255.f, _background.g / 255.f, _background.b / 255.f, _background.a / 255.f};
            if (ImGui::ColorEdit4("Color", clr, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel))
            {
                _background.r = clr[0] * 255;
                _background.g = clr[1] * 255;
                _background.b = clr[2] * 255;
                _background.a = clr[3] * 255;
            }
        }
    }

    void Scene::imgui_menu()
    {
        _mode = Mode::Undefined;

        constexpr float tabw = 90.f;
        ImGui::SetNextItemWidth(tabw);
        if (ImGui::BeginTabItem(ICON_FA_GEAR " Setup"))
        {
            _mode = Mode::Setup;
            BeginDefaultMenu("wsmnu");
            EndDefaultMenu();
            ImGui::EndTabItem();
        }
        ImGui::SetNextItemWidth(tabw);
        if (ImGui::BeginTabItem(ICON_FA_BRUSH " Edit"))
        {
            _mode = Mode::Objects;
            if (auto* lyr = active_layer())
            {
                lyr->imgui_workspace_menu();
            }
            ImGui::EndTabItem();
        }
        ImGui::SetNextItemWidth(tabw);
        if (ImGui::BeginTabItem(ICON_FA_BOX_ARCHIVE " Prefabs"))
        {
            _mode = Mode::Prefab;
            SceneFactory::instance().imgui_workspace_menu();
            ImGui::EndTabItem();
        }
    }

    void Scene::imgui_workspace()
    {
        if (_mode == Mode::Prefab)
        {
            return SceneFactory::instance().imgui_workspace();
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

            if (_mode == Mode::Objects)
            {
                if (auto* lyr = active_layer())
                {
                    lyr->imgui_workspace(params, _drag);
                }
            }
        }
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

    SceneLayer* Scene::active_layer()
    {
        if (size_t(_edit._active_layer) < _layers.size())
            return _layers[_edit._active_layer];
        return nullptr;
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
