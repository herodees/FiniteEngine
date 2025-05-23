#include "scene.hpp"
#include "utils/dialog_utils.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"
#include "ecs/base.hpp"

namespace fin
{
    Scene::Scene()
    {

    }

    Scene::~Scene()
    {
    }

    void Scene::init(std::string_view root)
    {
        _factory.set_root(std::string(root));
        ecs::register_base_components(_factory);
        _factory.load();

        ecs::Base::storage().on_update().connect<&Scene::on_new_position>(this);
        ecs::Base::storage().on_destroy().connect<&Scene::on_destroy_position>(this);
    }

    const std::string& Scene::get_path() const
    {
        return _path;
    }

    void Scene::activate_grid(const Recti& screen)
    {
        _camera.size     = screen.size();

        auto canvas_size = _canvas.get_size();
        auto new_canvas_size = _camera.size;
        if (canvas_size != new_canvas_size)
        {
            _canvas.create(new_canvas_size.x, new_canvas_size.y);
        }
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
        layer->_index  = int32_t(_layers.size()) - 1;
        layer->resize(_size);
        return layer->_index;
    }

    void Scene::delete_layer(int32_t n)
    {
        delete _layers[n];
        _layers.erase(_layers.begin() + n);
        for (; n < _layers.size(); ++n)
            _layers[n]->_index = (int32_t)n;
    }


    Vec2i Scene::get_active_size() const
    {
        return _camera.size;
    }

    Vec2i Scene::get_scene_size() const
    {
        return _size;
    }

    void Scene::set_camera_position(Vec2f pos, float speed)
    {
        if (_camera.position == pos)
            return;

        _goto = pos;
        _goto_speed = speed;
        if (speed == 1.f)
        {
            _camera.position = _goto;
            _goto_speed      = 0;
        }
    }

    void Scene::set_camera_center(Vec2f pos, float speed)
    {
        set_camera_position(pos - _camera.size / 2, speed);
    }

    Vec2f Scene::get_camera_center() const
    {
        return _camera.position + _camera.size / 2;
    }

    const Camera& Scene::get_camera() const
    {
        return _camera;
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

    void Scene::render(Renderer& dc)
    {
        if (!_canvas)
            return;

        dc.set_debug(_mode == Mode::Objects);

        // Draw on the canvas
        BeginTextureMode(*_canvas.get_texture());
        ClearBackground(_background);


        dc.set_origin({(float)_camera.position.x, (float)_camera.position.y});
        BeginMode2D(dc._camera);

        for (auto* el : _layers)
        {
            el->render(dc);
        }

        if (auto* lyr = active_layer())
        {
            lyr->render_edit(dc);
        }

        EndMode2D();
        EndTextureMode();

        dc.set_debug(false);
    }

    void Scene::update(float dt)
    {
        update_camera_position(dt);

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
        _factory.clear_old_entities();
        _size={1024, 1024};
    }

    RenderTexture2D& Scene::canvas()
    {
        return _canvas;
    }

    ComponentFactory& Scene::factory()
    {
        return _factory;
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

    void Scene::serialize(msg::Var& ar)
    {
        ar.set_item("version", SCENE_VERSION);

        ar.set_item("width", _size.x);
        ar.set_item("height", _size.y);

        msg::Var bgclr;
        bgclr.push_back((int)_background.r);
        bgclr.push_back((int)_background.g);
        bgclr.push_back((int)_background.b);
        bgclr.push_back((int)_background.a);
        ar.set_item("background", bgclr);

        msg::Var layers;
        for (auto lyr : _layers)
        {
            msg::Var layer;
            lyr->serialize(layer);
            layers.push_back(layer);
        }
        ar.set_item("layers", layers);
    }

    void Scene::deserialize(msg::Var& ar)
    {
        clear();
        _size.x = ar.get_item("width").get(0);
        _size.y = ar.get_item("height").get(0);
        auto bg = ar.get_item("background");
        if (bg.is_array() && bg.size() == 4)
        {
            _background.r = bg[0].get(0);
            _background.g = bg[1].get(0);
            _background.b = bg[2].get(0);
            _background.a = bg[3].get(0);
        }
        auto layers = ar.get_item("layers");
        for (auto ly : layers.elements())
        {
             add_layer(SceneLayer::create(ly, this));
        }
    }

    void Scene::load(std::string_view path)
    {
        _path = path;
        auto*     txt = LoadFileText(_path.c_str());

        /*
        msg::Pack pack;
        pack.data().assign(txt, txt + size);
        auto ar = pack.get();
        deserialize(ar);
        */
        msg::Var doc;
        doc.from_string((const char*)txt);
        deserialize(doc);

        UnloadFileText(txt);

    }

    void Scene::save(std::string_view path, bool change_path)
    {
        std::string p(path);
        if (change_path)
        {
            _path = p;
        }

        msg::Var doc;
        serialize(doc);
        std::string str;
        doc.to_string(str);
        SaveFileData(p.c_str(), str.data(), str.size());
        return;


        msg::Pack pack;
        serialize(pack);
        auto ar = pack.data();
        SaveFileData(p.c_str(), pack.data().data(), pack.data().size());
    }

    void Scene::start(bool st)
    {
        if (st)
            _mode = Mode::Running;
        else
            _mode = Mode::Undefined;
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
            case Mode::Prefabs:

                if (_edit_prefabs)
                    _factory.imgui_props(this);
                else if (auto* lyr = active_layer())
                {
                    ImGui::PushID("lypt");
                    lyr->imgui_update(false);
                    ImGui::PopID();
                }
                break;
        }

        ImGui::End();
    }

    void Scene::imgui_items()
    {
        if (_show_properties)
        { // Get main viewport
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2         center   = viewport->GetCenter();
            ImVec2 win_size(400, 350);
            ImVec2 win_pos = ImVec2(center.x - win_size.x * 0.5f, center.y - win_size.y * 0.5f);
            ImGui::SetNextWindowPos(win_pos, ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(win_size, ImGuiCond_Appearing);

            if (ImGui::Begin("Scene Properties",
                             &_show_properties,
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
            {
                imgui_props_setup();
            }
            ImGui::End();
        }


        if (_mode == Mode::Prefab)
        {
            if (ImGui::Begin("Items"))
            {
                SceneFactory::instance().imgui_items();
            }
            ImGui::End();
        }
        else if (_mode == Mode::Prefabs)
        {
            if (ImGui::Begin("Explorer"))
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    _edit_prefabs = true;
                }

                _factory.imgui_items(this);
            }
            ImGui::End();

            if (ImGui::Begin("Scene"))
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    _edit_prefabs = false;
                }

                imgui_scene();
            }
            ImGui::End();
        }
    }

    void Scene::on_new_position(Registry& reg, Entity ent)
    {
        auto& base = ecs::Base::storage().get(ent);
    }

    void Scene::on_destroy_position(Registry& reg, Entity ent)
    {
        auto& base = ecs::Base::storage().get(ent);
    }

    void Scene::imgui_scene()
    {
        if (ImGui::BeginChildFrame(ImGui::GetID("lyrssl"), {-1, 100}, 0))
        {
            int n = 0;
            for (auto* ly : _layers)
            {
                if (ImGui::LineSelect(ImGui::GetID(ly), _active_layer == n)
                        .Space()
                        .PushStyle(ImStyle_Button, 1)
                        .Text(ly->is_hidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE)
                        .PopStyle()
                        .Space()
                        .PushColor(ly->color())
                        .Text(ly->icon())
                        .Space()
                        .Text(ly->name())
                        .Spring()
                        .PopColor()
                        .Text(_active_layer == n ? ICON_FA_BRUSH : "")
                        .End())
                {
                    _active_layer = n;
                    if (ImGui::Line().HoverId() == 1)
                    {
                        ly->hide(!ly->is_hidden());
                    }
                }
                ++n;
            }
        }
        ImGui::EndChildFrame();

        if (auto* lyr = active_layer())
        {
            ImGui::PushID("lyit");
            lyr->imgui_update(true);
            ImGui::PopID();
        }
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
                    if (ImGui::LineSelect(ImGui::GetID(ly), _active_layer == n)
                            .Space()
                            .PushStyle(ImStyle_Button, 1)
                            .Text(ly->is_hidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE)
                            .PopStyle()
                            .Space()
                            .PushColor(ly->color())
                            .Text(ly->icon())
                            .Space()
                            .Text(ly->name())
                            .Spring()
                            .PopColor()
                            .Text(_active_layer == n ? ICON_FA_BRUSH : "")
                            .End())
                    {
                        _active_layer = n;
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
            if (ImGui::Button(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Resize..."))
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
            ImGui::SameLine();
            ImGui::Text("Size: %.0fpx x %.0fpx", _size.x, _size.y);
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
                    _active_layer = move_layer(_active_layer, false);
                }
                if (ImGui::Line().HoverId() == 3)
                {
                    _active_layer = move_layer(_active_layer, true);
                }
                if (ImGui::Line().HoverId() == 4 && active_layer())
                {
                    delete_layer(_active_layer);
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
                if (ImGui::MenuItem(ICON_FA_BOX " Object layer"))
                    add_layer(SceneLayer::create(SceneLayer::Type::Object));
                ImGui::EndPopup();
            }

            if (ImGui::BeginChildFrame(-2, {-1, 100}))
            {
                int n = 0;
                for (auto* ly : _layers)
                {
                    if (ImGui::LineSelect(ImGui::GetID(ly), _active_layer == n)
                            .PushColor(ly->color())
                            .Text(ly->icon())
                            .Space()
                            .Text(ly->name())
                            .End())
                    {
                        _active_layer = n;
                    }
                    ++n;
                }
            }
            ImGui::EndChildFrame();
            if (size_t(_active_layer) < _layers.size())
            {
                _layers[_active_layer]->imgui_setup();
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

    void Scene::update_camera_position(float dt)
    {
        if (_goto_speed > 0)
        {
            _camera.position += (_goto - _camera.position) * _goto_speed * dt;

            if (std::abs(_camera.position.x - _goto.x) < 0.1 && std::abs(_camera.position.y - _goto.y) < 0.1)
            {
                _goto_speed      = 0;
                _camera.position = _goto;
            }
        }

        Rectf screen(_camera.position.x, _camera.position.y, _camera.size.x, _camera.size.y);
        for (auto* ly : _layers)
        {
            ly->activate(screen);
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

        ImGui::SetNextItemWidth(tabw);
        if (ImGui::BeginTabItem(ICON_FA_BOXES_PACKING " Prefabs"))
        {
            _mode = Mode::Prefabs;
            _factory.imgui_show(this);
            ImGui::EndTabItem();
        }
    }

    void Scene::imgui_filemenu()
    {
        if (ImGui::BeginPopupModal("Scene Properties", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui_props_setup();

            if (ImGui::LineItem("itms", { -1, ImGui::GetFrameHeight() })
                .Spring()
                .PushStyle(ImStyle_Button, 1)
                .Text("   OK   ")
                .PopStyle()
                .Space()
                .End())
            {
                if (ImGui::Line().HoverId())
                    ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
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
            params.pos          = ImGui::GetWindowPos();


            auto cur     = ImGui::GetCursorPos();
            params.dc    = ImGui::GetWindowDrawList();
            auto mpos    = ImGui::GetMousePos();
            params.mouse = {mpos.x - params.pos.x - cur.x + ImGui::GetScrollX(),
                            mpos.y - params.pos.y - cur.y + ImGui::GetScrollY()};

            params.dc->AddImage((ImTextureID)&_canvas.get_texture()->texture,
                                {cur.x + params.pos.x, cur.y + params.pos.y},
                                {cur.x + params.pos.x + _canvas.get_width(), cur.y + params.pos.y + _canvas.get_height()},
                                {0, 1},
                                {1, 0});

            params.scroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
            params.pos.x -= ImGui::GetScrollX();
            params.pos.y -= ImGui::GetScrollY();


            ImGui::InvisibleButton("Canvas", ImVec2(size.x, size.y));
            auto itemid = ImGui::GetItemID();

            activate_grid(Recti(0, 0, visible_size.x, visible_size.y));
            if (get_camera().position.x != params.scroll.x || get_camera().position.y != params.scroll.y)
            {
                set_camera_position({params.scroll.x, params.scroll.y});
            }
            _drag.update(params.mouse.x, params.mouse.y);

         //   if (_mode == Mode::Objects)
            {
                if (auto* lyr = active_layer())
                {
                    lyr->imgui_workspace(params, _drag);
                }
            }

            ImGui::ScrollWhenDragging({-1, -1}, ImGuiMouseButton_Right, itemid);
        }
    }

    void Scene::imgui_show_properties(bool show)
    {
        _show_properties = true;
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
        for (size_t n = 0; _layers.size(); ++n)
            _layers[n]->_index = (int32_t)n;
        return layer;
    }

    SceneLayer* Scene::active_layer()
    {
        if (size_t(_active_layer) < _layers.size())
            return _layers[_active_layer];
        return nullptr;
    }

    SceneLayer* Scene::find_layer(std::string_view name) const
    {
        for (auto* ly : _layers)
        {
            if (ly->name() == name)
                return ly;
        }
        return nullptr;
    }


    void DragData::update(float x, float y)
    {
        if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
        {
            _active = true;
            _begin  = {x, y};
            _delta  = {};
        }

        if (_active && (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1)))
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
