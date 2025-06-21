#include "scene.hpp"
#include "utils/dialog_utils.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"
#include "ecs/builtin.hpp"
#include "ecs/core.hpp"
#include <rlgl.h>
#include "application.hpp"

namespace fin
{
    ImGui::CanvasParams _s_canvas;

    class SceneProperties : public ImGui::Dialog
    {
        enum
        {
            PropertyUndefined = -1,
            PropertyBasic     = 0,
            PropertyLayout,
            PropertySystem,
        };
    public:
        SceneProperties(Scene& scene) : _scene(scene)
        {
        }

        bool OnUpdate() override
        {
            if (selected == PropertyUndefined)
            {
                _new_size = _scene.GetSceneSize();
                selected = PropertyBasic;
            }

            // Left
            if (ImGui::BeginChild("left pane", ImVec2(140, 0), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX, 0))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15)); // Wider/taller padding
                ImGui::Dummy({1, 1});
                if (ImGui::Selectable(" " ICON_FA_GEAR " Basic", selected == PropertyBasic))
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
                                "Scene size will resize all layers without destroying objects. Background color sets "
                                "the "
                                "scene background.");

                    Vec2i old_size(_scene.GetSceneSize());
                    ImGui::InputInt2("Size", &_new_size.x);
                    if (old_size != _new_size)
                    {
                        ImGui::SameLine();
                        if (ImGui::Button(ICON_FA_ROTATE_LEFT " Revert"))
                            selected = PropertyUndefined;
                        ImGui::SameLine();
                        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
                        {
                            _scene.SetSize(_new_size);
                            selected = PropertyUndefined;
                        }
                    }

                    auto bg = _scene.GetBackgroundColor();
                    float clr[4] = {bg.r / 255.f, bg.g / 255.f, bg.b / 255.f, bg.a / 255.f};
                    if (ImGui::ColorEdit4("Color", clr, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel))
                    {
                        bg.r = clr[0] * 255;
                        bg.g = clr[1] * 255;
                        bg.b = clr[2] * 255;
                        bg.a = clr[3] * 255;
                        _scene.SetBackgroundColor(bg);
                    }
                }
                else if (selected == PropertyLayout)
                {
                    show_header("Layout Settings",
                                "You can create and manage custom layers in the scene to control drawing and logic "
                                "order. "
                                "Layers can represent different types like regions, sprites, or object layers. "
                                "Use this to organize your scene more efficiently and define how elements overlap.");

                    if (ImGui::BeginChild("left pane",
                                          ImVec2(250, 0),
                                          ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX,
                                          ImGuiWindowFlags_NoBackground))
                    {
                        _scene.GetLayers().ImguiSetup();
                    }
                    ImGui::EndChild();
                    ImGui::SameLine();
                    if (ImGui::BeginChild("right pane", ImVec2(0, 0), ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoBackground))
                    {
                        if (auto* lyr = _scene.GetLayers().GetActiveLayer())
                        {
                            lyr->ImguiSetup();
                        }
                    }
                    ImGui::EndChild();
                }
                else if (selected == PropertySystem)
                {
                    show_header("System Settings",
                                "Manage all active systems in the scene. Systems define logic and behavior using the "
                                "ECS "
                                "(Entity-Component-System) model.\n"
                                "You can add, remove, or reorder systems to control how your scene updates and "
                                "reacts.");

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
                                _active_system = _scene.GetSystems().MoveSystem(_active_system, false);
                            }
                            if (ImGui::Line().HoverId() == 3)
                            {
                                _active_system = _scene.GetSystems().MoveSystem(_active_system, true);
                            }
                            if (ImGui::Line().HoverId() == 4)
                            {
                                _scene.GetSystems().DeleteSystem(_active_system);
                            }
                        }

                        if (ImGui::BeginPopup("SystemMenu"))
                        {
                            _active_system = _scene.GetSystems().ImguiMenu();
                            ImGui::EndPopup();
                        }

                        if (ImGui::BeginChildFrame(ImGui::GetID("sysi"), {-1, -1}))
                        {
                            _scene.GetSystems().ImguiSystems(&_active_system);
                        }
                        ImGui::EndChildFrame();
                    }
                    ImGui::EndChild();
                    ImGui::SameLine();
                    if (ImGui::BeginChild("right pane", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground))
                    {
                        if (_scene.GetSystems().IsValid(_active_system))
                        {
                            _scene.GetSystems().GetSystem(_active_system)->ImguiSetup();
                        }
                    }

                    ImGui::EndChild();
                }
            }
            ImGui::EndChild();

            return _valid;
        }

    private:
        int   _active_system{-1};
        int   selected = PropertyUndefined;
        Vec2i _new_size;
        bool   _valid{true};
        Scene& _scene;
    };



    Scene::Scene() : SystemManager(*this), ComponentFactory(*this), LayerManager(*this)
    {

    }

    Scene::~Scene()
    {
        for (auto& plg : _plugins)
        {
            UnloadPlugin(plg.second);
        }

    }

    void Scene::Init(std::string_view root)
    {
        GetFactory().SetRoot(std::string(root));
        GetFactory().Load();
        ecs::RegisterCoreSystems(GetSystems());
        GetSystems().AddDefaults();
    }

    const std::string& Scene::GetPath() const
    {
        return _path;
    }

    void Scene::ActicateGrid(const Recti& screen)
    {
        _camera.size     = screen.size();

        auto canvas_size = _canvas.get_size();
        auto new_canvas_size = _camera.size;
        if (canvas_size != new_canvas_size)
        {
            _canvas.create(new_canvas_size.x, new_canvas_size.y);
        }
    }

    void Scene::SetSize(Vec2f size)
    {
        if (_size != size)
        {
            _size = size;
            GetLayers().SetSize(size);
        }
    }

    Vec2i Scene::GetActiveSize() const
    {
        return _camera.size;
    }

    Vec2i Scene::GetSceneSize() const
    {
        return _size;
    }

    void Scene::SetCameraPosition(Vec2f pos, float speed)
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

    void Scene::SetCameraCenter(Vec2f pos, float speed)
    {
        SetCameraPosition(pos - _camera.size / 2, speed);
    }

    Vec2f Scene::GetCameraCenter() const
    {
        return _camera.position + _camera.size / 2;
    }

    const Camera& Scene::GetCamera() const
    {
        return _camera;
    }

    Color Scene::GetBackgroundColor() const
    {
        return _background;
    }

    void Scene::SetBackgroundColor(Color clr)
    {
        _background = clr;
    }

    IGamePlugin* Scene::GetPlugin(std::string_view guid) const
    {
        for (const auto& plg : _plugins)
        {
            if (plg.second->GetInfo().guid == guid)
            {
                return plg.second;
            }
        }
        return nullptr;
    }

    void Scene::Render(Renderer& dc)
    {
        if (!_canvas)
            return;

        // Draw on the canvas
        BeginTextureMode(*_canvas.get_texture());
        ClearBackground(_background);

        rlSetBlendFactorsSeparate(0x0302, 0x0303, 1, 0x0303, 0x8006, 0x8006);
        BeginBlendMode(BLEND_CUSTOM_SEPARATE);

        //dc.set_origin({(float)_camera.position.x, (float)_camera.position.y});
        dc._camera.target.x = _camera.position.x;
        dc._camera.target.y = _camera.position.y;
        dc._camera.zoom = _camera.zoom;
        BeginMode2D(dc._camera);

        GetLayers().Render(dc);

        EndMode2D();
        EndTextureMode();

    }

    void Scene::Update(float dt)
    {
        if (_mode == SceneMode::Play)
        {
            for (auto& plug : _plugins)
                plug.second->OnPreUpdate(dt);
        }

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
        GetLayers().Activate(screen);

        if (_mode == SceneMode::Play)
        {
            for (auto& plug : _plugins)
                plug.second->OnUpdate(dt);

            GetSystems().Update(dt);
            GetLayers().Update(dt);
        }
    }

    void Scene::PostUpdate(float dt)
    {
        if (_mode == SceneMode::Play)
        {
            for (auto& plug : _plugins)
                plug.second->OnPostUpdate(dt);
        }
    }

    void Scene::Init()
    {
        if (_was_inited)
            return;
        GetLayers().Init();
        GetSystems().OnStartRunning();
        for (auto& plug : _plugins)
            plug.second->OnStart();
        _was_inited = true;
    }

    void Scene::Deinit()
    {
        if (!_was_inited)
            return;
        for (auto& plug : _plugins)
            plug.second->OnStop();
        GetSystems().OnStopRunning();
        GetLayers().Deinit();
        _was_inited = false;
    }

    void Scene::FixedUpdate(float dt)
    {
        if (_mode == SceneMode::Play)
        {
            for (auto& plug : _plugins)
                plug.second->OnFixedUpdate(dt);
            GetSystems().FixedUpdate(dt);
            GetLayers().FixedUpdate(dt);
        }
    }

    void Scene::Clear()
    {
        GetLayers().Clear();
        GetFactory().ClearOldEntities();
        _size={1024, 1024};
    }

    RenderTexture2D& Scene::GetCanvas()
    {
        return _canvas;
    }

    ComponentFactory& Scene::GetFactory()
    {
        return *this;
    }

    SystemManager& Scene::GetSystems()
    {
        return *this;
    }

    LayerManager& Scene::GetLayers()
    {
        return *this;
    }

    void Scene::Serialize(msg::Var& ar)
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
        GetLayers().Serialize(layers);
        ar.set_item("layers", layers);

        msg::Var plugins;
        for (const auto& plg : _plugins)
        {
            msg::Var plugin;
            plugin.set_item("guid", plg.second->GetInfo().guid);
            plugin.set_item("name", plg.second->GetInfo().name);
            plugin.set_item("version", plg.second->GetInfo().version);
            plg.second->OnSerialize(plugin); 
            plugins.push_back(plugin);
        }
        ar.set_item("plugins", plugins);

    }

    void Scene::Deserialize(msg::Var& ar)
    {
        Clear();
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
        GetLayers().Deserialize(layers);

        auto plugins = ar.get_item("plugins");
        if (plugins.is_array())
        {
            for (auto& plg : plugins.elements())
            {
                if (auto* plug = GetPlugin(plg.get_item("guid").str()))
                {
                    if (plug->OnDeserialize(plg))
                    {
                        TraceLog(LOG_INFO, "Loading plugin %s", plug->GetInfo().name.data());
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Loading plugin %s failed", plug->GetInfo().name.data());
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "Undefined Plugin %s {%s}",
                             plg.get_item("name").c_str(),
                             plg.get_item("guid").c_str());
                }
            }
        }
    }

    void Scene::Load(std::string_view path)
    {
        _path = path;
        auto*     txt = LoadFileText(_path.c_str());

        msg::Var doc;
        doc.from_string((const char*)txt);
        Deserialize(doc);

        UnloadFileText(txt);
    }

    void Scene::Save(std::string_view path, bool change_path)
    {
        std::string p(path);
        if (change_path)
        {
            _path = p;
        }

        msg::Var doc;
        Serialize(doc);
        std::string str;
        doc.to_string(str);
        SaveFileData(p.c_str(), str.data(), str.size());
        GetFactory().Save();
    }

    void Scene::SetMode(SceneMode st)
    {
        _mode = st;
    }

    SceneMode Scene::GetMode() const
    {
        return _mode;
    }

    void Scene::ImguiWorkspace()
    {
        if (ImGui::Begin("Workspace"))
        {
            ImguiMenu();

            ImVec2 pos  = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (_mode == SceneMode::Prefab)
            {
                GetFactory().ImguiWorkspace(this);
            }
            else
            {
                if (auto* lyr = GetActiveLayer())
                {
                    lyr->ImguiWorkspaceMenu(_s_canvas);
                }
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    _edit_prefabs = false;
                }

                pos  = ImGui::GetCursorScreenPos();
                size = ImGui::GetContentRegionAvail();

                if (ImGui::BeginCanvas("SceneCanvas", ImVec2(-1, -1), _s_canvas))
                {
                    ActicateGrid(Recti(0, 0, _s_canvas.canvas_size.x, _s_canvas.canvas_size.y));
                    _camera.position = _s_canvas.ScreenToWorld(_s_canvas.canvas_pos);
                    _camera.zoom     = _s_canvas.zoom;
                    if (gSettings.grid_snap)
                    {
                        _s_canvas.snap_grid.x = float(gSettings.grid_snapx);
                        _s_canvas.snap_grid.y = float(gSettings.grid_snapy);
                    }
                    else
                    {
                        _s_canvas.snap_grid = {};
                    }
               

                    ImGui::GetWindowDrawList()->AddImage((ImTextureID)&_canvas.get_texture()->texture,
                                                         {_s_canvas.canvas_pos.x, _s_canvas.canvas_pos.y},
                                                         {_s_canvas.canvas_pos.x + _s_canvas.canvas_size.x,
                                                          _s_canvas.canvas_pos.y + _s_canvas.canvas_size.y},
                                                         {0, 1},
                                                         {1, 0});

                    if (auto* lyr = GetActiveLayer())
                    {
                        lyr->ImguiWorkspace(_s_canvas);
                    }

                    ImGui::DrawGrid(_s_canvas, {128, 128});
                    ImGui::DrawOrigin(_s_canvas, -1);
                    ImGui::DrawRuler(_s_canvas, {128, 128});
                    ImGui::EndCanvas();
                }
            }

            if (auto* lyr = GetActiveLayer())
            {
                if (_mode == SceneMode::Scene)
                {
                    ImGui::GetWindowDrawList()
                        ->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), lyr->GetColor(), 0.0f, ImDrawFlags_None, 1);
                }
            }

        }
        ImGui::End();
    }

    void Scene::ImguiProps()
    {
        if (!ImGui::Begin("Properties"))
        {
            ImGui::End();
            return;
        }

        if (_edit_prefabs)
            GetFactory().ImguiProperties(this);
        else if (auto* lyr = GetActiveLayer())
        {
            ImGui::PushID("lypt");
            lyr->ImguiUpdate(false);
            ImGui::PopID();
        }

        ImGui::End();
    }

    void Scene::ImguiItems()
    {
        if (ImGui::Begin("Explorer"))
        {
            if (GetFactory().IsPrefabMode())
            {
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                {
                    _edit_prefabs = true;
                }
            }
            else
            {
                _edit_prefabs = false;
            }
            GetFactory().ImguiItems(this);
        }
        ImGui::End();

        if (ImGui::Begin("Scene"))
        {
            if (_mode != SceneMode::Prefab && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                _edit_prefabs = false;
            }

            GetLayers().ImguiScene();
        }
        ImGui::End();
        
    }

    void Scene::ImguiMenu()
    {
        if (_mode == SceneMode::Prefab)
        {
            GetFactory().ImguiMenu(this);
        }
        else
        {
            if (ImGui::LineItem("WorkspaceTabs", {-1, ImGui::GetFrameHeight()})
                    .Space()
                    .PushStyle(ImStyle_Header, 1)
                    .Tooltip("Open Scene in Debug mode")
                    .Space()
                    .Text(ICON_FA_PLAY " Debug")
                    .Space()
                    .PopStyle()
                    .Spring()
                    .Text(GetPath().c_str())
                    .Space()
                    .End())
            {
                if (ImGui::Line().HoverId() == 1)
                {
                    std::string runtime_file("assets/___run___.map");
                    Save(runtime_file, false);
                    run_current_process({"/scene=\"" + runtime_file + "\""});
                }
            }
        }
    }

    void Scene::ImguiShowProperties(bool show)
    {
      //  _show_properties = true;
        ImGui::Dialog::ShowOnce<SceneProperties>("Scene Properties", {800, 600}, 0, *this);
    }

    bool Scene::UnloadPlugin(IGamePlugin* plug)
    {
        if (!plug)
            return false;

        GetRegister().RemoveComponentInfos(plug);
        plug->Release();
        return true;
    }

    bool Scene::LoadPlugin(const std::string& dir, const std::string& plugin)
    {
        DynamicLibrary lib;
        if (lib.Load(dir, plugin))
        {
            if (auto fn = lib.GetFunction<GamePluginInfo*()>("GetGamePluginInfoProc"))
            {
                auto* info = fn();
                TraceLog(LOG_INFO, "    > %s v%s by %s  [%s]", info->name.data(), info->version.data(), info->author.data(), info->guid.data());
            }
            else
            {
                return false;
            }

            if (auto fn = lib.GetFunction<bool(GameAPI*, ImguiAPI*)>("InitGamePluginProc"))
            {
                if (!fn(&gGameAPI, &gImguiAPI))
                {
                    return false;
                }
            }

            if (auto fn = lib.GetFunction<IGamePlugin*()>("CreateGamePluginProc"))
            {
                if (auto plug = fn())
                {
                    _plugins.emplace_back(std::move(lib), plug);
                    return true;
                }
            }
        }
        return false;
    }

} // namespace fin
