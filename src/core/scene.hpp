#pragma once

#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_object.hpp"
#include "shared_resource.hpp"

namespace fin
{
    class Scene
    {
        friend class SceneObject;
        struct Params;

    public:
        enum Mode
        {
            Undefined,
            Map,
            Objects,
            Regions,
        };

        struct Params
        {
            ImDrawList* dc;
            ImVec2      mouse;
            ImVec2      pos;
            ImVec2      scroll;
            float       mouse_distance2(ImVec2 pos) const;
        };

        struct IsoObject
        {
            int32_t                 _depth        : 31;
            bool                    _depth_active : 1;
            Line<float>             _origin;
            Region<float>           _bbox;
            SceneObject*            _ptr;
            std::vector<IsoObject*> _back;
            int32_t                 depth_get();
        };

        struct IsoManager
        {
            uint32_t                _iso_pool_size{};
            std::vector<IsoObject>  _iso_pool;
            std::vector<IsoObject*> _iso;
            void                    update(lq::SpatialDatabase& db, const Recti& region, SceneObject* edit);
        };

        Scene();
        ~Scene();

        bool  setup_background_texture(const std::filesystem::path& file);
        void  activate_grid(const Recti& screen);
        Vec2i get_active_grid_min() const;
        Vec2i get_active_grid_max() const;
        Vec2i get_scene_size() const;
        Vec2i get_grid_size() const;

        void         object_serialize(SceneObject* obj, msg::Writer& ar);
        SceneObject* object_deserialize(msg::Value& ar);
        void         object_insert(SceneObject* obj);
        void         object_remove(SceneObject* obj);
        void         object_select(SceneObject* obj);
        void         object_destroy(SceneObject* obj);

        void         region_serialize(SceneRegion* obj, msg::Writer& ar);
        SceneRegion* region_deserialize(msg::Value& ar);
        void         region_insert(SceneRegion* obj);
        void         region_remove(SceneRegion* obj);
        void         region_select(SceneRegion* obj);
        void         region_destroy(SceneRegion* obj);

        SceneObject* object_find_at(Vec2f position, float radius);
        SceneRegion* region_find_at(Vec2f position);

        template <typename CB>
        void for_each_active(CB cb);

        void render(Renderer& dc);
        void update(float dt);
        void clear();
        void generate_navmesh();

        void serialize(msg::Pack& ar);
        void deserialize(msg::Value& ar);

        void open();
        void save();

        void on_imgui_props();
        void on_imgui_props_object();
        void on_imgui_props_region();
        void on_imgui_props_map();
        void on_imgui_menu();
        void on_imgui_workspace();
        void on_imgui_workspace_object(Params& params);
        void on_imgui_workspace_map(Params& params);

    private:
        std::vector<Texture2D>               _grid_texture;
        std::vector<Surface>                 _grid_surface;
        std::vector<std::pair<size_t, bool>> _grid_active;
        std::vector<SceneObject*>            _scene;
        std::vector<SceneRegion*>            _regions;
        IsoManager                           _iso_manager;
        lq::SpatialDatabase                  _spatial_db;
        NavMesh::PathFinder                  _pathfinder;
        SceneObject*                         _edit_object{};
        SceneObject*                         _selected_object{};
        SceneRegion*                         _selected_region{};
        int32_t                              _active_prototype_type{};
        uint32_t                             _active_prototype{0};
        int32_t                              _active_point{-1};
        Vec2f                                _drag_begin;
        Vec2f                                _drag_delta;
        bool                                 _drag_active{};
        bool                                 _add_point{};
        bool                                 _move_point{};
        bool                                 _debug_draw_grid{};
        bool                                 _debug_draw_navmesh{};
        bool                                 _debug_draw_regions{};
        bool                                 _debug_draw_object{};
        bool                                 _edit_region{};
        Mode                                 _mode{Mode::Map};
        Recti                                _active_region;
        Vec2i                                _grid_size;
        RenderTexture2D                      _canvas;
    };


    template <typename CB>
    inline void Scene::for_each_active(CB cb)
    {
        for (auto* el : _iso_manager._iso)
        {
            if (cb(el->_ptr))
                return;
        }
    }

} // namespace fin
