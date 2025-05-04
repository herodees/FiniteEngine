#pragma once

#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_layer.hpp"
#include "scene_object.hpp"
#include "shared_resource.hpp"

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
        };

        struct EditData
        {
            std::string       _buffer;
            SceneRegion*      _selected_region{};
            int32_t           _active_point{-1};
            int32_t           _active_layer{0};
            bool              _add_point{};
            bool              _move_point{};
        };

        Scene();
        ~Scene();

        const std::string& get_path() const;

        void activate_grid(const Recti& screen);
        void activate_grid(const Vec2f& origin);

        void        set_size(Vec2f size);
        int32_t     add_layer(SceneLayer* layer);
        void        delete_layer(int32_t n);
        int32_t     move_layer(int32_t layer, bool up);
        SceneLayer* active_layer();

        Vec2i get_active_grid_size() const;
        Vec2f get_active_grid_center() const;
        Vec2i get_scene_size() const;

        void        name_object(ObjectBase* obj, std::string_view name);
        ObjectBase* find_object_by_name(std::string_view name);

        SceneRegion* region_find_at(Vec2f position);

        void             render(Renderer& dc);
        void             update(float dt);
        void             clear();
        RenderTexture2D& canvas();

        void serialize(msg::Pack& ar);
        void deserialize(msg::Value& ar);

        void load(std::string_view path);
        void save(std::string_view path);

        void imgui_explorer();
        void imgui_props();
        void imgui_props_object();
        void imgui_props_setup();
        void imgui_menu();
        void imgui_workspace();

    private:
        std::vector<SceneRegion*>                                                       _regions;
        std::vector<SceneLayer*>                                                        _layers;
        std::unordered_map<std::string, ObjectBase*, std::string_hash, std::equal_to<>> _named_object;
        DragData                                                                        _drag{};
        EditData                                                                        _edit{};
        bool                                                                            _edit_region{};
        Mode                                                                            _mode{Mode::Setup};
        Recti                                                                           _active_region;
        RenderTexture2D                                                                 _canvas;
        std::string                                                                     _path;
        Vec2f                                                                           _size;
        Color                                                                           _background{BLACK};
    };


} // namespace fin
