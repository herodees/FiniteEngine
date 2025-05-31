#include "scene.hpp"
#include "utils/dialog_utils.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"
#include "ecs/base.hpp"
#include "ecs/core.hpp"

namespace fin
{
    ImGui::CanvasParams _s_canvas;

    Scene::Scene() : _systems(*this)
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
        ecs::register_core_systems(_systems);
        _systems.add_defaults();

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

    void Scene::render(Renderer& dc)
    {
        if (!_canvas)
            return;

        // Draw on the canvas
        BeginTextureMode(*_canvas.get_texture());
        ClearBackground(_background);


        //dc.set_origin({(float)_camera.position.x, (float)_camera.position.y});
        dc._camera.target.x = _camera.position.x;
        dc._camera.target.y = _camera.position.y;
        dc._camera.zoom = _camera.zoom;
        BeginMode2D(dc._camera);

        for (auto* el : _layers)
        {
            el->render(dc);
        }

        EndMode2D();
        EndTextureMode();

    }

    void Scene::update(float dt)
    {
        update_camera_position(dt);

        if (_mode == SceneMode::Play)
        {
            _systems.update(dt);
            std::for_each(_layers.begin(), _layers.end(), [dt](auto* lyr) { lyr->update(dt); });
        }
    }

    void Scene::init()
    {
        std::for_each(_layers.begin(), _layers.end(), [](auto* lyr) { lyr->init(); });
        _systems.on_start_runing();
    }

    void Scene::deinit()
    {
        _systems.on_stop_runing();
        std::for_each(_layers.rbegin(), _layers.rend(), [](auto* lyr) { lyr->deinit(); });
    }

    void Scene::fixed_update(float dt)
    {
        if (_mode == SceneMode::Play)
        {
            _systems.fixed_update(dt);
            std::for_each(_layers.begin(), _layers.end(), [dt](auto* lyr) { lyr->fixed_update(dt); });
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

    SystemManager& Scene::systems()
    {
        return _systems;
    }

    ScriptFactory& Scene::scripts()
    {
        return _scripts;
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
        _size.x = ar.get_item("width").get(0.f);
        _size.y = ar.get_item("height").get(0.f);
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
        _factory.save();
    }

    void Scene::set_mode(SceneMode st)
    {
        _mode = st;
    }

    SceneMode Scene::get_mode() const
    {
        return _mode;
    }

    void Scene::imgui_work()
    {
        if (ImGui::Begin("Workspace"))
        {
            imgui_menu();

            ImVec2 pos  = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginChildFrame(ImGui::GetID("cnvs"),
                                       {-1, -1},
                                       ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
            {
                imgui_workspace();
            }

            ImGui::EndChildFrame();

            if (auto* lyr = active_layer())
            {
                if (_mode == SceneMode::Scene)
                {
                    ImGui::GetWindowDrawList()
                        ->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), lyr->color(), 0.0f, ImDrawFlags_None, 1);
                }
            }

        }
        ImGui::End();
    }

    void Scene::imgui_props()
    {
        if (!ImGui::Begin("Properties"))
        {
            ImGui::End();
            return;
        }

        if (_edit_prefabs)
            _factory.imgui_props(this);
        else if (auto* lyr = active_layer())
        {
            ImGui::PushID("lypt");
            lyr->imgui_update(false);
            ImGui::PopID();
        }

        ImGui::End();
    }

    void Scene::imgui_setup()
    {
        if (_show_properties)
        { // Get main viewport
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2         center   = viewport->GetCenter();
            ImVec2         win_size(800, 600);
            ImVec2         win_pos = ImVec2(center.x - win_size.x * 0.5f, center.y - win_size.y * 0.5f);
            ImGui::SetNextWindowPos(win_pos, ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(win_size, ImGuiCond_Appearing);

            if (ImGui::Begin("Scene Properties", &_show_properties, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
            {
                imgui_props_setup();
            }
            ImGui::End();
        }
    }

    void Scene::imgui_items()
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
            if (_mode != SceneMode::Prefab && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                _edit_prefabs = false;
            }

            imgui_scene();
        }
        ImGui::End();
        
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
        if (ImGui::BeginChild(ImGui::GetID("lyrssl"),
                              {-1, 100},
                              ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY))
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
                        .Text(_active_layer == n ? ICON_FA_CIRCLE_DOT : "")
                        .Space()
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
        ImGui::EndChild();

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
        enum
        {
            PropertyUndefined= -1,
            PropertyBasic = 0,
            PropertyLayout,
            PropertySystem,
        };
        static int   selected = PropertyUndefined;
        static Vec2i new_size;
        if (selected == PropertyUndefined)
        {
            new_size = _size;
            selected = PropertyBasic;
        }

        // Left

        if (ImGui::BeginChild("left pane",
                              ImVec2(140, 0),
                              ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX,
                              0))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15)); // Wider/taller padding
            ImGui::Dummy({1,1});
            if (ImGui::Selectable( " " ICON_FA_GEAR " Basic", selected == PropertyBasic))
                selected = PropertyBasic;

            if (ImGui::Selectable(" " ICON_FA_LAYER_GROUP " Layout", selected == PropertyLayout))
                selected = PropertyLayout;

            if (ImGui::Selectable(" " ICON_FA_GEARS " System", selected == PropertySystem))
                selected = PropertySystem;
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();


        ImGui::SameLine();

        // Right
        if (ImGui::BeginChild("item view",
                              ImVec2(0, 0),
                              ImGuiChildFlags_AlwaysUseWindowPadding,
                              ImGuiWindowFlags_NoBackground)) // Leave room for 1 line below us
        {
            auto show_header = [](const char* label, const char* desc)
                {
                ImGui::NewLine();
                ImGui::SetWindowFontScale(1.3f); // Only affects current window
                ImGui::Text(label);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::NewLine();
                ImGui::TextWrapped(desc);
                ImGui::NewLine();
                };

            if (selected == PropertyBasic)
            {
                show_header("Basic Settings",
                            "Scene size will resize all layers without destroying objects. Background color sets the "
                            "scene background.");

                Vec2i old_size(_size);
                ImGui::InputInt2("Size", &new_size.x);
                if (old_size != new_size)
                {
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_ROTATE_LEFT " Revert"))
                        selected = PropertyUndefined;
                    ImGui::SameLine();
                    if(ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
                    {
                        set_size(new_size);
                        selected = PropertyUndefined;
                    }
                }

                float clr[4] = {_background.r / 255.f, _background.g / 255.f, _background.b / 255.f, _background.a / 255.f};
                if (ImGui::ColorEdit4("Color", clr, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel))
                {
                    _background.r = clr[0] * 255;
                    _background.g = clr[1] * 255;
                    _background.b = clr[2] * 255;
                    _background.a = clr[3] * 255;
                }
            }
            else if (selected == PropertyLayout)
            {
                show_header("Layout Settings",
                            "You can create and manage custom layers in the scene to control drawing and logic order. "
                            "Layers can represent different types like regions, sprites, or object layers. "
                            "Use this to organize your scene more efficiently and define how elements overlap.");

                if (ImGui::BeginChild("left pane",
                                      ImVec2(250, 0),
                                      ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX,
                                      ImGuiWindowFlags_NoBackground))
                {
                    if (ImGui::LineItem(ImGui::GetID("mnu"), {-1, ImGui::GetFrameHeight()})
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
                        if (ImGui::MenuItem(ICON_FA_IMAGE " Sprite layer"))
                            _active_layer = add_layer(SceneLayer::create(LayerType::Sprite));
                        if (ImGui::MenuItem(ICON_FA_MAP_LOCATION_DOT " Region layer"))
                            _active_layer = add_layer(SceneLayer::create(LayerType::Region));
                        if (ImGui::MenuItem(ICON_FA_BOX " Object layer"))
                            _active_layer = add_layer(SceneLayer::create(LayerType::Object));
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginChildFrame(ImGui::GetID("lyrs"), {-1, -1}))
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
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("right pane", ImVec2(0, 0), ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoBackground))
                {
                    if (size_t(_active_layer) < _layers.size())
                    {
                        _layers[_active_layer]->imgui_setup();
                    }
                }
                ImGui::EndChild();
            }
            else if (selected == PropertySystem)
            {
                show_header("System Settings",
                            "Manage all active systems in the scene. Systems define logic and behavior using the ECS "
                            "(Entity-Component-System) model.\n"
                            "You can add, remove, or reorder systems to control how your scene updates and reacts.");

                if (ImGui::BeginChild("left pane",
                                      ImVec2(250, 0),
                                      ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX,
                                      ImGuiWindowFlags_NoBackground))
                {
                    if (ImGui::LineItem(ImGui::GetID("mnu"), {-1, ImGui::GetFrameHeight()})
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
                            ImGui::OpenPopup("SystemMenu");
                        }

                        if (ImGui::Line().HoverId() == 2)
                        {
                            _active_system = _systems.move_system(_active_system, false);
                        }
                        if (ImGui::Line().HoverId() == 3)
                        {
                            _active_system = _systems.move_system(_active_system, true);
                        }
                        if (ImGui::Line().HoverId() == 4 && active_layer())
                        {
                            delete_layer(_active_system);
                        }
                    }

                    if (ImGui::BeginPopup("SystemMenu"))
                    {
                        _active_system = _systems.imgui_menu();
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginChildFrame(ImGui::GetID("sysi"), {-1, -1}))
                    {
                        _systems.imgui_systems(&_active_system);
                    }
                    ImGui::EndChildFrame();
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("right pane", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground))
                {
                    if (_systems.is_valid(_active_system))
                    {
                        _systems.get_system(_active_system)->imgui_setup();
                    }
                }
                
                ImGui::EndChild();
             }
        }
        ImGui::EndChild();

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

        Rectf screen(_camera.position.x, _camera.position.y, _camera.size.x / _camera.zoom, _camera.size.y / _camera.zoom);
        for (auto* ly : _layers)
        {
            ly->activate(screen);
        }
    }

    void Scene::imgui_menu()
    {
        if (_mode == SceneMode::Prefab)
        {
            _factory.imgui_menu(this);
        }
        else
        {
            if (ImGui::LineItem("WorkspaceTabs", {-1, ImGui::GetFrameHeight()})
                    .Space()
                    .PushStyle(ImStyle_Header, 1)
                    .Space()
                    .Text(ICON_FA_PLAY " Debug")
                    .Space()
                    .PopStyle()
                    .Spring()
                    .Text(get_path().c_str())
                    .Space()
                    .End())
            {
                if (ImGui::Line().HoverId() == 1)
                {
                    std::string runtime_file("assets/___run___.map");
                    save(runtime_file, false);
                    run_current_process({"/scene=\"" + runtime_file + "\""});
                }
            }
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
        if (_mode == SceneMode::Prefab)
        {
            _factory.imgui_workspace(this);
            return;
        }
        else
        {
            if (ImGui::BeginCanvas("SceneCanvas", ImVec2(-1, -1), _s_canvas))
            {
                activate_grid(Recti(0, 0, _s_canvas.canvas_size.x, _s_canvas.canvas_size.y));
                _camera.position = _s_canvas.ScreenToWorld(_s_canvas.canvas_pos);
                _camera.zoom     = _s_canvas.zoom;

                ImGui::GetWindowDrawList()->AddImage((ImTextureID)&_canvas.get_texture()->texture,
                                                     {_s_canvas.canvas_pos.x, _s_canvas.canvas_pos.y},
                                                     {_s_canvas.canvas_pos.x + _s_canvas.canvas_size.x,
                                                      _s_canvas.canvas_pos.y + _s_canvas.canvas_size.y},
                                                     {0, 1},
                                                     {1, 0});

                if (auto* lyr = active_layer())
                {
                    lyr->imgui_workspace(_s_canvas);
                }

                ImGui::DrawGrid(_s_canvas, {128, 128});
                ImGui::DrawOrigin(_s_canvas, -1);
                ImGui::DrawRuler(_s_canvas, {128, 128});
                ImGui::EndCanvas();
            }
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
        for (size_t n = 0; n < _layers.size(); ++n)
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

    std::span<SceneLayer*> Scene::layers()
    {
        return std::span<SceneLayer*>(_layers);
    }




} // namespace fin
