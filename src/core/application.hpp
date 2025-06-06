#pragma once

#include "include.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "shared_resource.hpp"

namespace fin
{
    struct Settings
    {
        std::string buffer;
        bool        visible_grid{0};
        bool        visible_isometric{0};
        bool        visible_collision{0};
        bool        visible_navgrid{0};
        bool        list_visible_items{};
    };

    extern Settings g_settings;

    class application : public std::enable_shared_from_this<application>
    {
    public:
        application();
        ~application();

        bool on_iterate();
        bool on_init(char* argv[], size_t argc);
        void on_deinit(bool result);

        bool cmd_attribute_exists(std::string_view cmd) const;
        std::string_view cmd_attribute_get(std::string_view cmd) const;

    private:
        void imgui_init(bool dark_theme);
        void imgui();
        void imgui_file_menu();

        std::span<char*>  _argv;
        Renderer          _renderer;
        Scene             _map;
        int32_t           _target_fps       = 60;
        float             _fixed_fps        = 30.0f;
        float             _max_fps          = 1200.0f;
        double            _fixed_time_step  = 1.0f / _fixed_fps;
        double            _max_time_step    = 1.0f / _max_fps;
        double            _current_time     = GetTime();
        double            _time_accumulator = 0.0;
    };
} // namespace fin
