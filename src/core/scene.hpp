#pragma once

#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include "ecs/behavior.hpp"
#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_layer.hpp"
#include "shared_resource.hpp"

namespace fin
{
    constexpr int32_t SCENE_VERSION = 1;

    struct Camera
    {
        Vec2f position;
        Vec2i size;
        float zoom{1.f};
    };

    enum class SceneMode
    {
        Play,
        Scene,
        Prefab,
    };

    class Scene
    {
        friend class SpriteSceneObject;

    public:
        Scene();
        ~Scene();

        const std::string& get_path() const;
        void               init(std::string_view root);

        void        set_size(Vec2f size);
        int32_t     add_layer(SceneLayer* layer);
        void        delete_layer(int32_t n);
        int32_t     move_layer(int32_t layer, bool up);
        SceneLayer* active_layer();
        SceneLayer* find_layer(std::string_view name) const;


        void          activate_grid(const Recti& screen);
        Vec2i         get_active_size() const;
        Vec2i         get_scene_size() const;
        void          set_camera_position(Vec2f pos, float speed = 1.f);
        void          set_camera_center(Vec2f pos, float speed = 1.f);
        Vec2f         get_camera_center() const;
        const Camera& get_camera() const;

        void                   init();
        void                   deinit();
        void                   render(Renderer& dc);
        void                   update(float dt);
        void                   fixed_update(float dt);
        void                   clear();
        RenderTexture2D&       canvas();
        ComponentFactory&      factory();
        SystemManager&         systems();
        ScriptFactory&         scripts();
        std::span<SceneLayer*> layers();

        void serialize(msg::Var& ar);
        void deserialize(msg::Var& ar);

        void load(std::string_view path);
        void save(std::string_view path, bool change_path = true);

        void      set_mode(SceneMode sm);
        SceneMode get_mode() const;

        void imgui_work();
        void imgui_props();
        void imgui_setup();
        void imgui_items();
        void imgui_menu();
        void imgui_filemenu();
        void imgui_workspace();
        void imgui_show_properties(bool show);

    private:
        void on_new_position(Registry& reg, Entity ent);
        void on_destroy_position(Registry& reg, Entity ent);
        void imgui_scene();
        void imgui_props_object();
        void imgui_props_setup();
        void update_camera_position(float dt);

        ComponentFactory         _factory;
        SystemManager            _systems;
        ScriptFactory            _scripts;
        std::vector<SceneLayer*> _layers;
        int32_t                  _active_layer{0};
        int32_t                  _active_system{0};
        bool                     _show_properties{};
        bool                     _edit_region{};
        bool                     _edit_prefabs{};
        SceneMode                _mode{SceneMode::Play};
        Vec2f                    _goto;
        float                    _goto_speed{};
        Camera                   _camera;
        RenderTexture2D          _canvas;
        std::string              _path;
        Vec2f                    _size{1024, 1024};
        Color                    _background{BLACK};
    };


} // namespace fin
