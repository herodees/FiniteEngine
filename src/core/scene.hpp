#pragma once

#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_layer.hpp"
#include "scene_object.hpp"
#include "scene_npc.hpp"
#include "shared_resource.hpp"
#include "ecs/component.hpp"

namespace fin
{
    struct Params
    {
        ImDrawList* dc;
        ImVec2      mouse;
        ImVec2      pos;
        ImVec2      scroll;
        float       mouse_distance2(ImVec2 pos) const
        {
            auto dx = mouse.x - pos.x;
            auto dy = mouse.y - pos.y;
            return dx * dx + dy * dy;
        }
    };

    struct DragData
    {
        Vec2f _begin;
        Vec2f _delta;
        bool  _active{};

        void update(float x, float y);
    };

    struct Camera
    {
        Vec2f position;
        Vec2i size;
    };

    class Scene
    {
        friend class SpriteSceneObject;

    public:
        enum Mode
        {
            Undefined,
            Running,
            Setup,
            Objects,
            Prefab,
            Prefabs,
        };

        Scene();
        ~Scene();

        const std::string& get_path() const;
        void               init();

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

        void        name_object(ObjectBase* obj, std::string_view name);
        ObjectBase* find_object_by_name(std::string_view name);

        void             render(Renderer& dc);
        void             update(float dt);
        void             clear();
        RenderTexture2D& canvas();

        void serialize(msg::Pack& ar);
        void deserialize(msg::Value& ar);

        void load(std::string_view path);
        void save(std::string_view path, bool change_path = true);

        void start(bool st);

        void imgui_props();
        void imgui_items();
        void imgui_menu();
        void imgui_workspace();

    private:
        void imgui_props_object();
        void imgui_props_setup();
        void update_camera_position(float dt);

        ComponentFactory                                                                _factory;
        std::vector<SceneLayer*>                                                        _layers;
        std::unordered_map<std::string, ObjectBase*, std::string_hash, std::equal_to<>> _named_object;
        DragData                                                                        _drag{};
        int32_t                                                                         _active_layer{0};
        bool                                                                            _edit_region{};
        Mode                                                                            _mode{Mode::Setup};
        Vec2f                                                                           _goto;
        float                                                                           _goto_speed{};
        Camera                                                                          _camera;
        RenderTexture2D                                                                 _canvas;
        std::string                                                                     _path;
        Vec2f                                                                           _size;
        Color                                                                           _background{BLACK};
    };


} // namespace fin
