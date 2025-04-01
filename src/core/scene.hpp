#pragma once

#include "include.hpp"
#include "shared_resource.hpp"
#include "scene_object.hpp"
#include "renderer.hpp"
#include "CDT.h"
#include "prototype.hpp"

namespace fin
{
    class Scene
    {
    public:
        enum edit_mode
        {
            map, navmesh, objects, prototype
        };

        struct Params
        {
            ImDrawList* dc;
            ImVec2 mouse;
            ImVec2 pos;
            ImVec2 scroll;
        };

        struct IsoObject
        {
            int32_t _depth : 31;
            bool _depth_active : 1;
            Line<float> _origin;
            Region<float> _bbox;
            SceneObject *_ptr;
            std::vector<IsoObject*> _back;
            int32_t depth_get();
        };

        Scene();
        bool build_graph();

        bool setup_background_texture(const std::filesystem::path& file);
        bool setup_navmesh();
        void activate_grid(const Recti& screen);
        Vec2i get_active_grid_min() const;
        Vec2i get_active_grid_max() const;
        Vec2i get_scene_size() const;
        Vec2i get_grid_size() const;

        void object_serialize(SceneObject* obj, msg::Writer& ar);
        SceneObject* object_deserialize(msg::Value& ar);
        void object_insert(SceneObject* obj);
        void object_remove(SceneObject* obj);
        void object_move(SceneObject* obj, float dx, float dy);
        void object_moveto(SceneObject* obj, float x, float y);
        void object_move(SceneObject* obj, const Vec2f& d);
        void object_moveto(SceneObject* obj, const Vec2f& p);

        void render(Renderer& dc);
        void update(float dt);
        void clear();

        void serialize(msg::Pack& ar);
        void deserialize(msg::Value& ar);

        void open();
        void save();

        void on_imgui_props();
        void on_imgui_props_navmesh();
        void on_imgui_props_object();
        void on_imgui_props_map();
        void on_imgui_props_prototype();
        void on_imgui_menu();
        void on_imgui_workspace();
        void on_imgui_workspace_navmesh(Params& params);
        void on_imgui_workspace_object(Params& params);
        void on_imgui_workspace_map(Params& params);
        void on_imgui_workspace_prototype(Params &params);

        void AddPoint(Vec2f pos);
        std::vector<const CDT::Triangle*> FindPath(Vec2f start, Vec2f goal);
        bool FindSharedEdge(const CDT::Triangle& tri, const CDT::Triangle* neighbor, Vec2f& outStart, Vec2f& outEnd);
        std::vector<std::pair<Vec2f, Vec2f>> ExtractPortals(const std::vector<const CDT::Triangle*>& trianglePath, Vec2f start, Vec2f goal);
        void EnsureCorrectPortalOrder(std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start);
        std::vector<Vec2f> ComputeSmoothPath(const std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start);
        const CDT::Triangle* FindTriangle(const Vec2f& point);
        Vec2f GetCentroid(const CDT::Triangle* tri) const;
        bool Contains(const Vec2f& point, const CDT::Triangle& tri) const;

        uint32_t _iso_pool_size{};
        std::vector<Texture2D> _grid_texture;
        std::vector<Surface> _grid_surface;
        std::vector<IsoObject> _iso_pool;
        std::vector<IsoObject*> _iso;
        std::vector<std::pair<size_t, bool>> _grid_active;
        std::vector<Vec2f> _navmesh_points;
        std::vector<SceneObject*> _scene;
        lq::SpatialDatabase _spatial_db;
        SceneObject* _edit_object{};

        int32_t _active_point{-1};
        bool _add_point{};
        bool _move_point{};
        bool _debug_draw_grid{};
        bool _debug_draw_navmesh{};
        bool _debug_draw_object{};
        bool _debug_draw_prototype{};
        edit_mode _mode{ edit_mode::map };

        Recti _active_region;
        Vec2i _grid_size;
        CDT::Triangulation<float> _cdt;
        RenderTexture2D _canvas;

        std::shared_ptr<Catalogue> _catalogue;
    };
}
