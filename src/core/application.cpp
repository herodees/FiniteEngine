#include "application.hpp"
#include "utils/dialog_utils.hpp"
#include "utils/lib_utils.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"
#include "ecs/builtin.hpp"
#include "imgui_internal.h"

#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES3)
#include "GLFW/glfw3.h"
#endif
#include <api/plugin.hpp>
#include <api/register.hpp>


namespace fin
{
    Settings gSettings;
    GameAPI  gGameAPI{};
    ImguiAPI gImguiAPI{};

    void Application::ImguiInit(bool dark_theme)
    {
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        if (dark_theme)
            ImGui::StyleColorsDark();
        else
            ImGui::StyleColorsLight();

        io.IniFilename = nullptr;

        // Cherry style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha                    = 1.0f;
        style.DisabledAlpha            = 0.6000000238418579f;
        style.WindowPadding            = ImVec2(3.0f, 3.0f);
        style.WindowRounding           = 7.0f;
        style.WindowBorderSize         = 1.0f;
        style.WindowMinSize            = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding            = 7.0f;
        style.ChildBorderSize          = 1.0f;
        style.PopupRounding            = 2.0f;
        style.PopupBorderSize          = 1.0f;
        style.FramePadding             = ImVec2(3.0f, 3.0f);
        style.FrameRounding            = 2.0f;
        style.FrameBorderSize          = 1.0f;
        style.ItemSpacing              = ImVec2(3.0f, 3.0f);
        style.ItemInnerSpacing         = ImVec2(3.0f, 3.0f);
        style.CellPadding              = ImVec2(3.0f, 3.0f);
        style.IndentSpacing            = 21.0f;
        style.ColumnsMinSpacing        = 3.0f;
        style.ScrollbarSize            = 16.0f;
        style.ScrollbarRounding        = 1.0f;
        style.GrabMinSize              = 20.0f;
        style.GrabRounding             = 3.0f;
        style.TabRounding              = 2.0f;
        style.TabBorderSize            = 0.0f;
        style.ColorButtonPosition      = ImGuiDir_Right;
        style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

        ImVec4* colors                         = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.22f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.23f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.32f, 0.32f, 0.33f, 0.36f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.14f, 0.44f, 0.74f, 0.26f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_Separator]             = ImVec4(0.00f, 0.00f, 0.00f, 0.22f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.32f, 0.32f, 0.33f, 1.00f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.41f, 0.68f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_DragDropTarget]        = ImVec4(0.95f, 0.95f, 0.15f, 1.00f);
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    }

    bool Application::OnInit(char* argv[], size_t argc)
    {
        _argv = decltype(_argv)(argv, argv + argc);

        const int screenWidth  = 1920;
        const int screenHeight = 1080;

#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES3)
#if defined(__APPLE__)
        glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_METAL);
#elif defined(_WIN32)
        glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_D3D11);
#endif
#endif
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(screenWidth, screenHeight, "Finite");
        InitAudioDevice();

        // load the font
#if __ANDROID__
        std::filesystem::path basePath = ""; // on Android we do not want to use basepath. Instead, assets are available at the root directory.
#else
        auto basePathPtr = GetWorkingDirectory();
        if (not basePathPtr)
        {
            return false;
        }
        const std::filesystem::path basePath = basePathPtr;
#endif
        
        _map.Init(_asset_path);
        InitApi();
        InitPlugins();
        auto path = CmdAttributeGet("/scene");

        SceneMode mode = SceneMode::Scene;
        if (!path.empty())
        {
            mode = SceneMode::Play;
            TraceLog(LOG_INFO, "SCENE LOAD: %s", path.data());
            _map.ActicateGrid({0, 0, GetScreenWidth(), GetScreenHeight()});
            _map.Load(path);
        }

        _map.SetMode(mode);

        if (_map.GetMode() != SceneMode::Play)
        {
            rlImGuiSetup(true);
            ImguiInit(true);
        }

        return true;
    }

    void Application::OnDeinit(bool result)
    {
        if (_map.GetMode() != SceneMode::Play)
        {
            rlImGuiShutdown();
        }

        CloseAudioDevice();
        CloseWindow();
    }

    bool Application::CmdAttributeExists(std::string_view cmd) const
    {
        for (std::string_view c : _argv)
        {
            auto n = c.find('=');
            c      = c.substr(0, n);
            if (c == cmd)
                return true;
        }
        return false;
    }

    std::string_view Application::CmdAttributeGet(std::string_view cmd) const
    {
        for (std::string_view c : _argv)
        {
            auto n = c.find('=');
            if (c.substr(0, n) == cmd)
            {
                auto out = c.substr(n + 1);
                if (out.back() == '"' && out.front() == '"')
                {
                    out.remove_prefix(1);
                    out.remove_suffix(1);
                }
                return out;
            }

        }
        return std::string_view();
    }

    void Application::Imgui()
    {
        static bool init_docking = true;

        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        flags |= ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", 0, flags);
        ImGui::PopStyleVar();

        if (ImGui::BeginMenuBar())
        {
            ImguiFileMenu();
            ImGui::EndMenuBar();
        }

        ImGuiIO& io           = ImGui::GetIO();
        ImGuiID  dockspace_id = ImGui::GetID("MyDockspace");

        if (init_docking)
        {
            init_docking = false;

            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id);    // Add empty node

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_list = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
            ImGuiID dock_id_comp = ImGui::DockBuilderSplitNode(dock_id_list, ImGuiDir_Down, 0.40f, NULL, &dock_id_list);
            ImGuiID dock_id_prop = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, NULL, &dock_main_id);
            ImGuiID dock_id_items = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, NULL, &dock_main_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, NULL, &dock_main_id);

            ImGui::DockBuilderDockWindow("Workspace", dock_id_bottom);
            ImGui::DockBuilderDockWindow("Properties", dock_id_prop);
            ImGui::DockBuilderDockWindow("Items", dock_id_items);
            ImGui::DockBuilderDockWindow("Explorer", dock_id_list);
            ImGui::DockBuilderDockWindow("Scene", dock_id_comp);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id);
        if (!init_docking)
        {
            _map.ImguiItems();
            _map.ImguiProps();
            _map.ImguiWorkspace();
            ImGui::Dialog::Update();

            //ImGui::ShowDemoWindow();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool Application::OnIterate()
    {
        _map.Init();
        while (!WindowShouldClose())
        {
            // Update
            //----------------------------------------------------------------------------------

            double newTime   = GetTime();
            double frameTime = newTime - _current_time;

            // Avoid spiral of death if CPU  can't keep up with target FPS
            if (frameTime > 0.25)
            {
                frameTime = 0.25;
            }

            _current_time = newTime;

            // Limit frame rate to avoid 100% CPU usage
            if (frameTime < _max_time_step)
            {
                float waitTime = (float)(_max_time_step - frameTime);
                WaitTime(1.0f / 1000.0f);
            }

            // Update open scene if exists
            //----------------------------------------------------------------------------------
            if (!_open_scene.empty())
            {
                _map.Deinit();
                _map.Load(_open_scene);
                _map.Init();
                _open_scene.clear();
            }

            // Input
            //----------------------------------------------------------------------------------

            PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

            _time_accumulator += frameTime;
            while (_time_accumulator >= _fixed_time_step)
            {
                // Fixed Update
                //--------------------------------------------------------------------------
                _map.FixedUpdate(_fixed_time_step);
                _time_accumulator -= _fixed_time_step;
            }

            // Drawing
            //----------------------------------------------------------------------------------
            if (_map.GetMode() == SceneMode::Play)
            {
                _map.ActicateGrid({0,0, GetScreenWidth(), GetScreenHeight()});
            }

            _map.Update(frameTime);

            BeginDrawing();

            _map.Render(_renderer);

            frameTime = frameTime + (GetTime() - newTime);
            _map.PostUpdate(frameTime);

            ClearBackground(BLACK);

            if (_map.GetMode() != SceneMode::Play)
            {
                rlImGuiBegin(frameTime);
                Imgui();
                rlImGuiEnd();
            }
            else
            {
                DrawTexturePro(_map.GetCanvas().texture.texture,
                               {0, 0, (float)_map.GetCanvas().texture.texture.width, -(float)_map.GetCanvas().texture.texture.height},
                               {0, 0, (float)_map.GetCanvas().texture.texture.width, (float)_map.GetCanvas().texture.texture.height},
                               {0, 0},
                               0,
                               WHITE);
            }

            EndDrawing();


            SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)
        }
        _map.Deinit();

        return true;
    }

    void Application::ImguiFileMenu()
    {
        std::string_view show_popup;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New"))
                {
                    _map.Clear();
                }

                char const* filter_patterns[2] = {"*.json", "*.atlas"};
                char const* filter_description = "Atlas file";
                bool        open_save_as       = false;

                if (ImGui::MenuItem("Open"))
                {
                    auto files = open_file_dialog("", "");
                    if (!files.empty())
                    {
                        _map.Deinit();
                        _map.Load(files[0]);
                        _map.Init();
                    }
                }
                if (ImGui::MenuItem("Save"))
                {
                    if (_map.GetPath().empty())
                    {
                        auto out = save_file_dialog("", "");
                        if (!out.empty())
                        {
                            _map.Save(out);
                        }
                    }
                    else
                    {
                        _map.Save(_map.GetPath());
                    }
                }
                if (ImGui::MenuItem("Save as") || open_save_as)
                {
                    auto out = save_file_dialog("", "");
                    if (!out.empty())
                    {
                        _map.Save(out);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scene"))
            {
                if (ImGui::MenuItem(ICON_FA_WRENCH " Properties", NULL))
                {
                   _map.ImguiShowProperties(true);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem(ICON_FA_BORDER_ALL " Visible grid", NULL, gSettings.visible_grid))
                {
                    gSettings.visible_grid = !gSettings.visible_grid;
                }
                if (ImGui::MenuItem(ICON_FA_MAP_LOCATION_DOT " Visible isometric guide", NULL, gSettings.visible_isometric))
                {
                    gSettings.visible_isometric = !gSettings.visible_isometric;
                }
                if (ImGui::MenuItem(ICON_FA_VECTOR_SQUARE " Visible collision", NULL, gSettings.visible_collision))
                {
                    gSettings.visible_collision = !gSettings.visible_collision;
                }
                if (ImGui::MenuItem(ICON_FA_MAP " Visible navigation grid", NULL, gSettings.visible_navgrid))
                {
                    gSettings.visible_navgrid = !gSettings.visible_navgrid;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (!show_popup.empty())
            ImGui::OpenPopup(show_popup.data());
    }

    void Application::InitApi()
    {
        gImguiAPI.PushID     = ImGui::PushID;
        gImguiAPI.PushPtrID  = ImGui::PushID;
        gImguiAPI.PopID      = ImGui::PopID;
        gImguiAPI.GetID      = ImGui::GetID;
        gImguiAPI.Text       = ImGui::Text;
        gImguiAPI.Checkbox   = ImGui::Checkbox;
        gImguiAPI.Button     = [](const char* id, Vec2f size) -> bool { return ImGui::Button(id, (ImVec2&)size); };
        gImguiAPI.Select     = [](const char* id, bool selected) -> bool { return ImGui::Selectable(id, selected); };
        gImguiAPI.BeginCombo = [](const char* id, const char* selected) -> bool
        { return ImGui::BeginCombo(id, selected); };
        gImguiAPI.EndCombo = ImGui::EndCombo;

        gGameAPI.version                = 1;
        gGameAPI.context                = this;
        gGameAPI.RegisterComponentInfo  = RegisterComponentInfo;
        gGameAPI.GetComponentInfoByType = GetComponentInfoType;
        gGameAPI.GetComponentInfoById   = GetComponentInfoId;
        gGameAPI.FindNamedEntity        = FindNamedEntity;
        gGameAPI.SetNamedEntity         = SetNamedEntity;
        gGameAPI.ClearNamededEntity     = ClearNamededEntity;
        gGameAPI.CreateEntity           = CreateEntity;
        gGameAPI.DestroyEntity          = DestroyEntity;
        gGameAPI.ValidEntity            = ValidEntity;
        gGameAPI.GetOldEntity           = GetOldEntity;
        gGameAPI.FindLayer              = FindLayer;
        gGameAPI.OpenScene              = OpenScene;

        RegisterBaseComponents(_map.GetFactory().GetRegister());
    }

    void Application::InitPlugins()
    {
#if defined(_WIN32)
        const char* ext = ".dll";
#elif defined(__APPLE__)
        const char* ext = ".dylib";
#else
        const char* ext = ".so";
#endif

        std::string plugin_dir = _asset_path + "plugins";
        auto        list       = LoadDirectoryFilesEx(plugin_dir.c_str(), ext, false);
        TraceLog(LOG_INFO, "GAME PLUGINS:");
        for (int i = 0; i < list.count; i++)
        {
            auto* file = GetFileNameWithoutExt(list.paths[i]);
            _map.LoadPlugin(plugin_dir, file);
        }
        UnloadDirectoryFiles(list);
    }

    bool Application::RegisterComponentInfo(AppHandle self, ComponentInfo* info)
    {
        if (!info)
            return false;
        self->_map.GetFactory().GetRegister().AddComponentInfo(info);
        return true;
    }

    ComponentInfo* Application::GetComponentInfoType(AppHandle self, StringView name)
    {
        return self->_map.GetFactory().GetRegister().GetComponentInfoByType(name);
    }

    ComponentInfo* Application::GetComponentInfoId(AppHandle self, StringView name)
    {
        return self->_map.GetFactory().GetRegister().GetComponentInfoById(name);
    }

    Entity Application::FindNamedEntity(AppHandle self, StringView name)
    {
        return self->_map.GetFactory().GetEntityByName(name);
    }

    bool Application::SetNamedEntity(AppHandle self, Entity ent, StringView name)
    {
        return self->_map.GetFactory().SetEntityName(ent, name);
    }

    void Application::ClearNamededEntity(AppHandle self, StringView name)
    {
        auto ent = self->_map.GetFactory().GetEntityByName(name);
        if (ent == entt::null)
            return;
        self->_map.GetFactory().SetEntityName(ent, {});
    }

    Entity Application::GetOldEntity(AppHandle self, Entity oldent)
    {
        return self->_map.GetFactory().GetOldEntity(oldent);
    }

    Layer* Application::FindLayer(AppHandle self, StringView name)
    {
        return self->_map.GetLayers().FindLayer(name);
    }

    bool Application::OpenScene(AppHandle self, StringView path)
    {
        if (path.empty())
            return false;

        self->_open_scene = self->_asset_path;
        self->_open_scene.append(path);
        if (!FileExists(self->_open_scene.c_str()))
        {
            TraceLog(LOG_ERROR, "Scene file not found: %s", self->_open_scene.c_str());
            self->_open_scene.clear();
            return false;
        }
        return true;
    }

    Entity Application::CreateEntity(AppHandle self)
    {
        return self->_map.GetFactory().GetRegister().Create();
    }

    void Application::DestroyEntity(AppHandle self, Entity ent)
    {
        self->_map.GetFactory().GetRegister().Destroy(ent);
    }

    bool Application::ValidEntity(AppHandle self, Entity ent)
    {
        return self->_map.GetFactory().GetRegister().Valid(ent);
    }

} // namespace fin
