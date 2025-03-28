#include "application.hpp"
#include "atlas.hpp"
#include "atlas_scene_object.hpp"
#include "atlas_scene_editor.hpp"

namespace fin
{
    application::application() 
    {
        _map.AddPoint({ 0, 0 });
        _map.AddPoint({ 800, 0 });
        _map.AddPoint({ 800, 600 });
        _map.AddPoint({ 0, 600 });

    }

    application::~application()
    {
    }

    void application::on_imgui_init(bool dark_theme)
    {
        if (dark_theme)
            ImGui::StyleColorsDark();
        else
            ImGui::StyleColorsLight();

        ImGui::GetIO().IniFilename = nullptr;

        // Cherry style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.6000000238418579f;
        style.WindowPadding = ImVec2(3.0f, 3.0f);
        style.WindowRounding = 7.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 7.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(3.0f, 3.0f);
        style.FrameRounding = 3.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(3.0f, 3.0f);
        style.ItemInnerSpacing = ImVec2(3.0f, 3.0f);
        style.CellPadding = ImVec2(3.0f, 3.0f);
        style.IndentSpacing = 21.0f;
        style.ColumnsMinSpacing = 3.0f;
        style.ScrollbarSize = 13.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabMinSize = 20.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;
        style.TabBorderSize = 1.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.68f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.99f, 0.99f, 0.99f, 0.05f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.27f, 0.32f, 0.45f, 0.17f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.07f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.44f, 0.14f, 0.26f, 0.97f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.38f, 0.65f, 0.91f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.38f, 0.65f, 0.91f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.11f, 0.52f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.45f, 0.20f, 0.30f, 0.76f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.41f, 0.35f, 0.40f, 0.47f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.11f, 0.52f, 1.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.45f, 0.20f, 0.30f, 0.76f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
        colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.07f, 0.08f, 0.09f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.45f, 0.20f, 0.30f, 0.43f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    SDL_AppResult application::on_event(SDL_Event* event)
	{
        ImGui_ImplSDL3_ProcessEvent(event);

        if (event->type == SDL_EVENT_QUIT)
        {
            _app_quit = SDL_APP_SUCCESS;
        }
        return SDL_APP_CONTINUE;
	}

    SDL_AppResult application::on_init()
    {
        // init the library, here we make a window so we only need the Video capabilities.
        if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            return return_fail();
        }

        // init TTF
        if (not TTF_Init()) {
            return return_fail();
        }
        // create a window

        _window = SDL_CreateWindow("SDL Minimal Sample", 1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        if (not _window) {
            return return_fail();
        }

        // create a renderer
        _renderer = SDL_CreateRenderer(_window, NULL);
        if (not _renderer) {
            return return_fail();
        }

        _active_renderer.handle = _renderer;

        _canvas.create(800, 600, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET);
        SDL_SetTextureBlendMode(_canvas.get_texture(), SDL_BLENDMODE_BLEND);

        // load the font
#if __ANDROID__
        std::filesystem::path basePath = "";   // on Android we do not want to use basepath. Instead, assets are available at the root directory.
#else
        auto basePathPtr = SDL_GetBasePath();
        if (not basePathPtr) {
            return return_fail();
        }
        const std::filesystem::path basePath = basePathPtr;
#endif
        _explorer.set_root(basePath.string());
        _explorer.select(basePath.string());
        _explorer.set_editor([&](std::string_view filename) { return createFileEdit(filename); });

        // load the SVG
        _txt.load_from_file(basePath / "gs_tiger.svg");

        Texture::load_shared((basePath / "gs_tiger.svg").string());
        Texture::load_shared((basePath / "gs_tiger.svg").string());

        // init SDL Mixer
        auto audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (not audioDevice) {
            return return_fail();
        }
        if (not Mix_OpenAudio(audioDevice, NULL)) {
            return return_fail();
        }

        // load the music
        auto musicPath = basePath / "the_entertainer.ogg";
        auto music = Mix_LoadMUS(musicPath.string().c_str());
        if (not music) {
            return return_fail();
        }

        // play the music (does not loop)
    //    Mix_PlayMusic(music, 0);

        // print some information about the window
        SDL_ShowWindow(_window);
        {
            int width, height, bbwidth, bbheight;
            SDL_GetWindowSize(_window, &width, &height);
            SDL_GetWindowSizeInPixels(_window, &bbwidth, &bbheight);
            SDL_Log("Window size: %ix%i", width, height);
            SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
            if (width != bbwidth) {
                SDL_Log("This is a highdpi environment.");
            }
        }

        SDL_SetRenderVSync(_renderer, 1);   // enable vysnc

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        on_imgui_init(true);
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(_window, _renderer);
        ImGui_ImplSDLRenderer3_Init(_renderer);

        SDL_Log("Application started successfully!");

        return SDL_APP_CONTINUE;
    }

    void application::on_deinit(SDL_AppResult result)
	{
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        _txt.clear();
        _canvas.clear();

        if(_renderer)
		    SDL_DestroyRenderer(_renderer);
        _active_renderer.handle = nullptr;

        if(_window)
		    SDL_DestroyWindow(_window);
		Mix_CloseAudio();
        if(_audioDevice)
		    SDL_CloseAudioDevice(_audioDevice);
	}

    void application::on_imgui_begin()
    {       
        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
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
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", 0, flags);
        ImGui::PopStyleVar();

        if (ImGui::BeginMenuBar())
        {
            on_imgui_menu();
            ImGui::EndMenuBar();
        }

        ImGuiIO& io = ImGui::GetIO();
        ImGuiID  dockspace_id = ImGui::GetID("MyDockspace");

        if (init_docking)
        {
            init_docking = false;

            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id); // Add empty node

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
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void application::on_imgui_end()
    {       
        // Rendering
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), _renderer);
    }

    FileEdit* application::createFileEdit(std::string_view filename)
    {
        auto ext = path_get_ext(filename);
        if (ext == ".atlas")
        {
            auto edit = new AtlasFileEdit();
            if (edit->on_init(filename))
            {
                return edit;
            }
            delete edit;
        }

        return nullptr;
    }

    SDL_AppResult application::on_iterate()
    {
        static Uint64 frequency = SDL_GetPerformanceFrequency();
        Uint64 current_time = SDL_GetPerformanceCounter();
        if (current_time <= _time)
            current_time = _time + 1;
        _delta_time = _time > 0 ? (float)((double)(current_time - _time) / frequency) : (float)(1.0f / 60.0f);
        _time = current_time;

        _map.update(_delta_time);
        _map.render(_active_renderer);

        if (_show_editor)
        {
            on_imgui_begin();

            // draw a color
            auto time = SDL_GetTicks() / 1000.f;
            auto red = (std::sin(time) + 1) / 2.0 * 255;
            auto green = (std::sin(time) + 1) / 2.0 * 255;
            auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;

            // Draw on the canvas
            SDL_SetRenderTarget(_renderer, _canvas.get_texture());
            SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255);
            SDL_RenderClear(_renderer);

            SDL_SetRenderDrawColor(_renderer, red, green, blue, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(_renderer);

            // Renderer uses the painter's algorithm to make the text appear above the image, we must render the image first.
            SDL_SetTextureBlendMode(_txt.get_texture(), SDL_BLENDMODE_BLEND);
            SDL_RenderTexture(_renderer, _txt.get_texture(), NULL, NULL);

            SDL_SetTextureBlendMode(_txt.get_texture(), eraseBlend);
            SDL_FRect fst{ 20.f,20.f,800.f,500.f };
            SDL_RenderTexture(_renderer, _txt.get_texture(), NULL, &fst);

            SDL_FRect fs1t{ 30.f,40.f,500.f,500.f };
            SDL_RenderTexture(_renderer, _txt.get_texture(), NULL, &fs1t);


            for (auto& tr : _map._cdt.triangles)
            {
                SDL_FPoint ps[4];
                for (int i = 0; i < 3; ++i)
                {
                    ps[i].x = _map._cdt.vertices[tr.vertices[i]].x;
                    ps[i].y = _map._cdt.vertices[tr.vertices[i]].y;
                }
                ps[3] = ps[0];
                SDL_SetRenderDrawColor(_renderer, 255, 255, 0, 255);
                SDL_RenderLines(_renderer, ps, 4);
            }
            SDL_SetRenderDrawColor(_renderer, 255, 0, 0, 255);
            SDL_RenderLines(_renderer, (SDL_FPoint*)_path.data(), _path.size());
            SDL_SetRenderDrawColor(_renderer, 0, 0, 255, 255);
            for (auto ln : _portals)
            {
                SDL_RenderLine(_renderer, ln.first.x, ln.first.y, ln.second.x, ln.second.y);
            }

            SDL_SetRenderDrawColor(_renderer, red, green, blue, SDL_ALPHA_OPAQUE);
            // Switch back to default rendering
            SDL_SetRenderTarget(_renderer, NULL);
            SDL_RenderClear(_renderer);

            SDL_SetTextureBlendMode(_canvas.get_texture(), SDL_BLENDMODE_BLEND_PREMULTIPLIED);
            SDL_SetTextureScaleMode(_canvas.get_texture(), SDL_SCALEMODE_NEAREST);
            // Draw the final canvas to screen
            SDL_RenderTexture(_renderer, _canvas.get_texture(), NULL, NULL);

            on_imgui();
            on_imgui_end();

            SDL_RenderPresent(_renderer);
        }
        else
        {

        }

        return _app_quit;
    }

    void application::on_imgui_menu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                char const* filter_patterns[2] = { "*.json", "*.atlas" };
                char const* filter_description = "Atlas file";
                bool        open_save_as = false;

                if (ImGui::MenuItem("Open"))
                {
                }
                if (ImGui::MenuItem("Save"))
                {
                }
                if (ImGui::MenuItem("Save as") || open_save_as)
                {
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void application::on_imgui_dialogs()
    {
        _explorer.render();

        if (!ImGui::Begin("Setup")) {
            ImGui::End();
            return;
        }
        ImGui::End();
    }

    void application::on_imgui_workspace()
    {
        if (!ImGui::Begin("Workspace")) {
            ImGui::End();
            return;
        }
        if (ImGui::BeginTabBar("WorkspaceTabs"))
        {
            _map.on_imgui_menu();

            ImGui::EndTabBar();
        }

        ImGui::Separator();

        if (ImGui::BeginChildFrame(-1, { -1,-1 },
            ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
        {
            _map.on_imgui_workspace();



        }
        ImGui::EndChildFrame();

        ImGui::End();
    }

    void application::on_imgui_properties()
    {
        if (!ImGui::Begin("Properties")) {
            ImGui::End();
            return;
        }

        _map.on_imgui_props();

        ImGui::End();
    }

 
}
