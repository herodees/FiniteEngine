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
    class ObjectSceneLayer : public ObjectLayer, public SceneLayer
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
        ~ObjectSceneLayer() final;

        void             Insert(Entity ent) final;
        void             Remove(Entity ent) final;
        void             MoveTo(Entity ent, Vec2f pos) final;
        void             Move(Entity ent, Vec2f pos) final;
        void             Update(void* obj);
        Entity           FindAt(Vec2f position) const final;
        Entity           FindActiveAt(Vec2f position) const final;
        std::span<Vec2i> FindPath(Vec2i from, Vec2i to) const final;
        bool             FindPath(Vec2i from, Vec2i to, std::vector<Vec2i>& path) const;
        Navmesh&         GetNavmesh();
        const Navmesh&   GetNavmesh() const;
        const SparseSet& GetObjects(bool active_only = false) const final;
        ObjectLayer*     Objects() final;

        void             Update(float dt) final;
        void             Render(Renderer& dc) final;
        void             Activate(const Rectf& region) final;
        void             Serialize(msg::Var& ar) final;
        void             Deserialize(msg::Var& ar) final;
        void             Clear() final;
        void             Resize(Vec2f size) final;

        bool ImguiWorkspace(ImGui::CanvasParams& canvas) final;
        bool ImguiWorkspaceMenu(ImGui::CanvasParams& canvas) final;
        void ImguiSetup() final;
        bool ImguiUpdate(bool items) final;

    protected:
        void UpdateNavmesh();
        void SelectEdit(Entity ent);

        SparseSet               _objects;
        SparseSet               _selected;
        Vec2i                   _grid_size{0, 0};
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

    void BeginDefaultMenu(const char* id, ImGui::CanvasParams& canvas);
    bool EndDefaultMenu(ImGui::CanvasParams& canvas);

} // namespace fin
