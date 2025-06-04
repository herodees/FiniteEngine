#pragma once

#include "include.hpp"
#include "scene_layer.hpp"
#include "utils/lquery.hpp"
#include "navmesh.hpp"

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class ObjectSceneLayer : public SceneLayer
    {
        struct IsoObject
        {
            int32_t                 _depth        : 31;
            bool                    _depth_active : 1;
            Line<float>             _origin;
            Region<float>           _bbox;
            Entity                  _ptr;
            std::vector<IsoObject*> _back;

            int32_t                 depth_get();
            void                    setup(Entity ent);
        };

    public:
        ObjectSceneLayer();
        ~ObjectSceneLayer() override;

        void   Serialize(msg::Var& ar) override;
        void   Deserialize(msg::Var& ar) override;
        void   Clear() override;
        void   Resize(Vec2f size) override;
        void   Activate(const Rectf& region) override;
        void   insert(Entity ent);
        void   remove(Entity ent);
        void   moveto(Entity ent, Vec2f pos);
        void   move(Entity ent, Vec2f pos);
        void   Update(void* obj);
        Entity find_at(Vec2f position) const;
        Entity find_active_at(Vec2f position) const;
        bool   find_path(Vec2i from, Vec2i to, std::vector<Vec2i>& path) const;
        void   select_edit(Entity ent);
        void   Update(float dt) override;
        void   Render(Renderer& dc) override;
        void   ImguiWorkspace(ImGui::CanvasParams& canvas) override;
        void   ImguiWorkspaceMenu() override;
        void   ImguiSetup() override;
        void   ImguiUpdate(bool items) override;
        Entity get_active(size_t n);
        size_t get_active_count() const;
        Navmesh& get_navmesh() { return _navmesh; }
        const Navmesh& get_navmesh() const { return _navmesh; }

    protected:
        void update_navmesh();

        entt::sparse_set        _objects;
        entt::sparse_set        _selected;
        Vec2i                   _grid_size{0,0};
        Vec2i                   _cell_size{16, 8};
        Vec2f                   _size;
        lq::SpatialDatabase     _spatial_db;
        std::vector<IsoObject>  _iso_pool;
        std::vector<IsoObject*> _iso;
        Navmesh                 _navmesh;
        uint32_t                _iso_pool_size{};
        int32_t                 _inflate{};
        Entity                  _edit{entt::null};
        Entity                  _drop{entt::null};
        bool                    _dirty_navmesh{};
    };

    void BeginDefaultMenu(const char* id);
    bool EndDefaultMenu();

} // namespace fin
