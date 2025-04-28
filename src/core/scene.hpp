#pragma once

#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_object.hpp"
#include "shared_resource.hpp"
#include "scene_layer.hpp"

namespace fin
{

    class Scene
    {
        friend class SpriteSceneObject;
        struct Params;

    public:
        enum Mode
        {
            Undefined,
            Running,
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
            IsoSceneObject*         _ptr;
            std::vector<IsoObject*> _back;
            int32_t                 depth_get();
        };

        struct IsoManager
        {
            uint32_t                  _iso_pool_size{};
            std::vector<IsoObject>    _iso_pool;
            std::vector<IsoObject*>   _iso;
            std::vector<BasicSceneObject*> _static;
            void update(lq::SpatialDatabase& db, const Recti& region, BasicSceneObject* edit);
        };

        struct DragData
        {
            Vec2f _begin;
            Vec2f _delta;
            bool  _active{};

            void update(float x, float y);
        };

        struct EditData
        {
            std::string       _buffer;
            BasicSceneObject* _edit_object{};
            BasicSceneObject* _selected_object{};
            SceneRegion*      _selected_region{};
            int32_t           _active_point{-1};
            int32_t           _active_layer{-1};
            bool              _add_point{};
            bool              _move_point{};
        };

        Scene();
        ~Scene();

        const std::string& get_path() const;

        bool  setup_background_texture(const std::filesystem::path& file);
        void  activate_grid(const Recti& screen);
        void  activate_grid(const Vec2f& origin);

        Vec2i get_active_grid_size() const;
        Vec2f get_active_grid_center() const;
        Vec2i get_active_grid_min() const;
        Vec2i get_active_grid_max() const;
        Vec2i get_scene_size() const;
        Vec2i get_grid_size() const;

        void              object_serialize(BasicSceneObject* obj, msg::Writer& ar);
        BasicSceneObject* object_deserialize(msg::Value& ar);
        void              object_insert(BasicSceneObject* obj);
        void              object_remove(BasicSceneObject* obj);
        void              object_select(BasicSceneObject* obj);
        void              object_destroy(BasicSceneObject* obj);
        void              object_moveto(BasicSceneObject* obj, Vec2f pos);

        void         region_serialize(SceneRegion* obj, msg::Writer& ar);
        SceneRegion* region_deserialize(msg::Value& ar);
        void         region_insert(SceneRegion* obj);
        void         region_remove(SceneRegion* obj);
        void         region_select(SceneRegion* obj);
        void         region_destroy(SceneRegion* obj);
        void         region_moveto(SceneRegion* obj, Vec2f pos);

        void name_object(ObjectBase* obj, std::string_view name);
        ObjectBase* find_object_by_name(std::string_view name);

        SpriteSceneObject* object_find_at(Vec2f position, float radius);
        SceneRegion* region_find_at(Vec2f position);

        template <typename CB>
        void for_each_active(CB cb);

        void render(Renderer& dc);
        void update(float dt);
        void clear();
        void generate_navmesh();
        RenderTexture2D& canvas();

        void serialize(msg::Pack& ar);
        void deserialize(msg::Value& ar);

        void load(std::string_view path);
        void save(std::string_view path);

        void on_imgui_props();
        void on_imgui_props_object();
        void on_imgui_props_region();
        void on_imgui_props_map();
        void on_imgui_menu();
        void on_imgui_workspace();
        void on_imgui_workspace_object(Params& params);
        void on_imgui_workspace_region(Params& params);
        void on_imgui_workspace_map(Params& params);

        int32_t move_layer(int32_t layer, bool up);

    private:
        std::vector<Texture2D>                                                          _grid_texture;
        std::vector<Surface>                                                            _grid_surface;
        std::vector<std::pair<size_t, bool>>                                            _grid_active;
        std::vector<BasicSceneObject*>                                                  _scene;
        std::vector<SceneRegion*>                                                       _regions;
        std::vector<SceneLayer*>                                                        _layers;
        std::unordered_map<std::string, ObjectBase*, std::string_hash, std::equal_to<>> _named_object;
        IsoManager                                                                      _iso_manager;
        lq::SpatialDatabase                                                             _spatial_db;
        NavMesh::PathFinder                                                             _pathfinder;
        DragData                                                                        _drag{};
        EditData                                                                        _edit{};
        bool                                                                            _debug_draw_grid{};
        bool                                                                            _debug_draw_navmesh{};
        bool                                                                            _debug_draw_regions{};
        bool                                                                            _debug_draw_object{};
        bool                                                                            _edit_region{};
        Mode                                                                            _mode{Mode::Map};
        Recti                                                                           _active_region;
        Vec2i                                                                           _grid_size;
        RenderTexture2D                                                                 _canvas;
        std::string                                                                     _path;

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
