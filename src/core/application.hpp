#pragma once

#include "include.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "shared_resource.hpp"
#include "api/register.hpp"

namespace fin
{
    struct Settings
    {
        std::string buffer;
        bool        visible_grid{};
        bool        visible_isometric{};
        bool        visible_collision{};
        bool        visible_navgrid{};
        bool        list_visible_items{};
        bool        grid_snap{};
        int         grid_snapx{4};
        int         grid_snapy{4};
    };

    extern Settings gSettings;

    class Application : public std::enable_shared_from_this<Application>
    {
    public:
        Application()  = default;
        ~Application() = default;

        bool OnIterate();
        bool OnInit(char* argv[], size_t argc);
        void OnDeinit(bool result);

        bool             CmdAttributeExists(std::string_view cmd) const;
        std::string_view CmdAttributeGet(std::string_view cmd) const;

    private:
        void Imgui();
        void ImguiInit(bool dark_theme);
        void ImguiFileMenu();
        void InitApi();
        void InitPlugins();

        static bool             RegisterComponentInfo(AppHandle self, ComponentInfo* info);
        static ComponentInfo*   GetComponentInfoType(AppHandle self, StringView name);
        static ComponentInfo*   GetComponentInfoId(AppHandle self, StringView name);
        static Entity           CreateEntity(AppHandle self);
        static void             DestroyEntity(AppHandle self, Entity ent);
        static bool             ValidEntity(AppHandle self, Entity ent);
        static Entity           FindNamedEntity(AppHandle self, StringView name);
        static bool             SetNamedEntity(AppHandle self, Entity ent, StringView name);
        static void             ClearNamededEntity(AppHandle self, StringView name);
        static Entity           GetOldEntity(AppHandle self, Entity oldent);
        static Layer*           FindLayer(AppHandle self, StringView name);
        static bool             OpenScene(AppHandle self, StringView path);

        std::span<char*> _argv;

        Renderer    _renderer;
        Scene       _map;
        std::string _asset_path = "./assets/";
        std::string _open_scene;
        int32_t     _target_fps       = 60;
        float       _fixed_fps        = 30.0f;
        float       _max_fps          = 1200.0f;
        double      _fixed_time_step  = 1.0f / _fixed_fps;
        double      _max_time_step    = 1.0f / _max_fps;
        double      _current_time     = GetTime();
        double      _time_accumulator = 0.0;
    };
} // namespace fin
