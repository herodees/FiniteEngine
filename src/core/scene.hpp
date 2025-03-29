#pragma once

#include "include.hpp"
#include "shared_resource.hpp"
#include "scene_object.hpp"
#include "CDT.h"

namespace fin
{
	class scene
	{
	public:
		enum edit_mode
		{
			map, navmesh, objects
		};

		struct workspace_params
		{
			ImDrawList* dc;
			ImVec2 mouse;
			ImVec2 pos;
			ImVec2 scroll;
		};

		struct object : SpatialItem
		{
			Rectf box;
			Vec2f col;
			Vec2f row;
			int32_t iso_depth{};
			std::vector<object*> iso_sprites_behind;
			Color clr;

			void setup();
			int32_t calculate_depth();
		};

		scene();
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
		void isometric_sort();
		void update_isometric();
		void clear();

		void serialize(msg::Pack& ar);
		void deserialize(msg::Value& ar);

		void on_imgui_props();
		void on_imgui_props_navmesh();
		void on_imgui_props_object();
		void on_imgui_props_map();
		void on_imgui_menu();
		void on_imgui_workspace();
		void on_imgui_workspace_navmesh(workspace_params& params);
		void on_imgui_workspace_object(workspace_params& params);
		void on_imgui_workspace_map(workspace_params& params);

		void AddPoint(Vec2f pos);
		std::vector<const CDT::Triangle*> FindPath(Vec2f start, Vec2f goal);
		bool FindSharedEdge(const CDT::Triangle& tri, const CDT::Triangle* neighbor, Vec2f& outStart, Vec2f& outEnd);
		std::vector<std::pair<Vec2f, Vec2f>> ExtractPortals(const std::vector<const CDT::Triangle*>& trianglePath, Vec2f start, Vec2f goal);
		void EnsureCorrectPortalOrder(std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start);
		std::vector<Vec2f> ComputeSmoothPath(const std::vector<std::pair<Vec2f, Vec2f>>& portals, Vec2f start);
		const CDT::Triangle* FindTriangle(const Vec2f& point);
		Vec2f GetCentroid(const CDT::Triangle* tri) const;
		bool Contains(const Vec2f& point, const CDT::Triangle& tri) const;

		std::filesystem::path _background_image;
		std::vector<Texture2D> _grid_texture;
		std::vector<Surface> _grid_surface;
		std::vector<std::pair<size_t, bool>> _grid_active;
		std::vector<Vec2f> _navmesh_points;
		std::vector<SceneObject*> _scene;
		std::vector<SpatialItem*> _active_objects;
		std::vector<object*> _objects;
		SpatialGrid _grid;
		SceneObject* _edit_object{};

		int32_t _active_point{-1};
		bool _add_point{};
		bool _move_point{};
		bool _debug_draw_grid{};
		bool _debug_draw_navmesh{};
		bool _debug_draw_object{};
		edit_mode _mode{ edit_mode::map };

		Recti _active_region;
		Vec2i _grid_size;
		CDT::Triangulation<float> _cdt;
		RenderTexture2D _canvas;
	};
}
