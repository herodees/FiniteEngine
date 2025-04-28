#include "application.hpp"

#include "atlas.hpp"
#include "imgui_internal.h"

#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES3)
#include "GLFW/glfw3.h"
#endif
#include <utils/dialog_utils.hpp>

namespace fin
{
    application::application()
    {
    }

    application::~application()
    {
    }

    void application::on_imgui_init(bool dark_theme)
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
        style.PopupRounding            = 0.0f;
        style.PopupBorderSize          = 1.0f;
        style.FramePadding             = ImVec2(3.0f, 3.0f);
        style.FrameRounding            = 3.0f;
        style.FrameBorderSize          = 0.0f;
        style.ItemSpacing              = ImVec2(3.0f, 3.0f);
        style.ItemInnerSpacing         = ImVec2(3.0f, 3.0f);
        style.CellPadding              = ImVec2(3.0f, 3.0f);
        style.IndentSpacing            = 21.0f;
        style.ColumnsMinSpacing        = 3.0f;
        style.ScrollbarSize            = 13.0f;
        style.ScrollbarRounding        = 3.0f;
        style.GrabMinSize              = 20.0f;
        style.GrabRounding             = 3.0f;
        style.TabRounding              = 3.0f;
        style.TabBorderSize            = 0.0f;
        style.ColorButtonPosition      = ImGuiDir_Right;
        style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);

        ImVec4* colors                         = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.32f, 0.32f, 0.33f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.63f, 0.68f, 0.73f, 0.26f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_Separator]             = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.32f, 0.32f, 0.33f, 1.00f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.11f, 0.59f, 0.93f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.00f, 0.47f, 0.78f, 1.00f);
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
        colors[ImGuiCol_DragDropTarget]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    }

    bool application::on_init(char* argv[], size_t argc)
    {
        _argv = decltype(_argv)(argv, argv + argc);

        const int screenWidth  = 1280;
        const int screenHeight = 720;

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

        _factory.set_root("./assets/");

        _factory.load_factory<SpriteSceneObject>(SpriteSceneObject::type_id, "Static");
        _factory.load_factory<SpriteSceneObject>("npc", "NPC");
        _factory.load_factory<SoundObject>(SoundObject::type_id, "Sound");

        auto path = cmd_attribute_get("/scene");
        if (!path.empty())
        {
            _editor   = false;
            TraceLog(LOG_INFO, "SCENE LOAD: %s", path.data());
            _map.load(path);
        }

        if (_editor)
        {
            rlImGuiSetup(true);
            on_imgui_init(true);
        }

        return true;
    }

    void application::on_deinit(bool result)
    {
        if (_editor)
        {
            rlImGuiShutdown();
        }

        CloseAudioDevice();
        CloseWindow();
    }

    bool application::cmd_attribute_exists(std::string_view cmd) const
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

    std::string_view application::cmd_attribute_get(std::string_view cmd) const
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

    void application::on_imgui()
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
            on_imgui_menu();
            ImGui::EndMenuBar();
        }

        ImGuiIO& io           = ImGui::GetIO();
        ImGuiID  dockspace_id = ImGui::GetID("MyDockspace");

        if (init_docking)
        {
            init_docking = false;

            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id);    // Add empty node

            ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
            ImGuiID dock_id_list = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
            ImGuiID dock_id_comp = ImGui::DockBuilderSplitNode(dock_id_list, ImGuiDir_Down, 0.30f, NULL, &dock_id_list);
            ImGuiID dock_id_prop = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, NULL, &dock_main_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, NULL, &dock_main_id);

            ImGui::DockBuilderDockWindow("Workspace", dock_id_bottom);
            ImGui::DockBuilderDockWindow("Properties", dock_id_prop);
            ImGui::DockBuilderDockWindow("Explorer", dock_id_list);
            ImGui::DockBuilderDockWindow("Setup", dock_id_comp);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id);
        if (!init_docking)
        {
            on_imgui_dialogs();
            on_imgui_properties();
            on_imgui_workspace();

            // ImGui::ShowDemoWindow();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool application::on_iterate()
    {
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

            // Input
            //----------------------------------------------------------------------------------

            PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

            _time_accumulator += frameTime;
            while (_time_accumulator >= _fixed_time_step)
            {
                // Fixed Update
                //--------------------------------------------------------------------------

                _time_counter += _fixed_time_step; // We count time (seconds)

                _time_counter += _fixed_time_step;
                _time_accumulator -= _fixed_time_step;
            }

            // Drawing
            //----------------------------------------------------------------------------------
            if (!_editor)
            {
                _map.activate_grid({0,0, GetScreenWidth(), GetScreenHeight()});
            }

            _map.update(frameTime);

            BeginDrawing();

            _map.render(_renderer);

            ClearBackground(BLACK);

            if (_editor)
            {
                rlImGuiBegin(frameTime);
                on_imgui();
                rlImGuiEnd();
            }
            else
            {
                DrawTexturePro(_map.canvas().texture.texture,
                               {0, 0, (float)_map.canvas().texture.texture.width, -(float)_map.canvas().texture.texture.height},
                               {0, 0, (float)_map.canvas().texture.texture.width, (float)_map.canvas().texture.texture.height},
                               {0, 0},
                               0,
                               WHITE);
            }

            EndDrawing();


            SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)
        }

        return true;
    }

    void application::on_imgui_menu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                char const* filter_patterns[2] = {"*.json", "*.atlas"};
                char const* filter_description = "Atlas file";
                bool        open_save_as       = false;

                if (ImGui::MenuItem("Open"))
                {
                    auto files = open_file_dialog("", "");
                    if (!files.empty())
                    {
                        _map.load(files[0]);
                    }
                }
                if (ImGui::MenuItem("Save"))
                {
                    if (_map.get_path().empty())
                    {
                        auto out = save_file_dialog("", "");
                        if (!out.empty())
                        {
                            _map.save(out);
                        }
                    }
                    else
                    {
                        _map.save(_map.get_path());
                    }
                }
                if (ImGui::MenuItem("Save as") || open_save_as)
                {
                    auto out = save_file_dialog("", "");
                    if (!out.empty())
                    {
                        _map.save(out);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void application::on_imgui_dialogs()
    {
        _factory.show_explorer();
        //_explorer.render();
    }

    void application::on_imgui_workspace()
    {
        if (!ImGui::Begin("Workspace"))
        {
            ImGui::End();
            return;
        }
        if (ImGui::BeginTabBar("WorkspaceTabs"))
        {
            _map.on_imgui_menu();

            _show_prefab = false;

            if (ImGui::BeginTabItem(ICON_FA_BOX_ARCHIVE " Prefabs"))
            {
                _show_prefab = true;
                _factory.show_menu();
                ImGui::EndTabItem();
            }

            if (!_map.get_path().empty())
            {
                if (ImGui::TabItemButton(" " ICON_FA_PLAY " ", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                {
                    run_current_process({"/scene=\"" + _map.get_path() + "\""});
                }
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();

        if (ImGui::BeginChildFrame(-1,
                                   {-1, -1},
                                   ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
        {
            if (_show_prefab)
            {
                _factory.show_workspace();
            }
            else
            {
                _factory.center_view();
                _map.on_imgui_workspace();
            }
        }
        ImGui::EndChildFrame();

        ImGui::End();
    }

    void application::on_imgui_properties()
    {
        if (!ImGui::Begin("Properties"))
        {
            ImGui::End();
            return;
        }

        if (_show_prefab)
        {
            _factory.show_properties();
        }
        else
        {
            _map.on_imgui_props();
        }

        ImGui::End();
    }


} // namespace fin
