#pragma once

#include "include.hpp"

#include "shared_resource.hpp"
#include "scene.hpp"
#include "file_explorer.hpp"
#include "renderer.hpp"

namespace fin
{
    class application : public std::enable_shared_from_this<application>
    {
    public:
        application();
        ~application();

        bool on_iterate();
        bool on_init(char *argv[], size_t argc);
        void on_deinit(bool result);

    private:
        void on_imgui_init(bool dark_theme);
        void on_imgui();
        void on_imgui_menu();
        void on_imgui_dialogs();
        void on_imgui_workspace();
        void on_imgui_properties();

        std::span<char *> _argv;
        FileExplorer _explorer;
        Renderer _renderer;
        Scene _map;
        int32_t _target_fps = 60;
        float _fixed_fps = 60.0f;
        float _max_fps = 1200.0f;
        double _time_counter = 0.0;
        double _fixed_time_step = 1.0f / _fixed_fps;
        double _max_time_step = 1.0f / _max_fps;
        double _current_time = GetTime();
        double _time_accumulator = 0.0;

        bool _show_editor{true};

        std::vector<const CDT::Triangle *> _triangles;
        std::vector<std::pair<Vec2f, Vec2f>> _portals;
    };
}
