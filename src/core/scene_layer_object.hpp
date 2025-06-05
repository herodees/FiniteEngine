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

        void             Insert(Entity ent);
        void             Remove(Entity ent);
        void             MoveTo(Entity ent, Vec2f pos);
        void             Move(Entity ent, Vec2f pos);
        void             Update(void* obj);
        Entity           FindAt(Vec2f position) const;
        Entity           FindActiveAt(Vec2f position) const;
        bool             FindPath(Vec2i from, Vec2i to, std::vector<Vec2i>& path) const;
        Navmesh&         GetNavmesh();
        const Navmesh&   GetNavmesh() const;
        const SparseSet& GetObjects(bool active_only = false) const;

        void Update(float dt) override;
        void Render(Renderer& dc) override;
        void Activate(const Rectf& region) override;
        void Serialize(msg::Var& ar) override;
        void Deserialize(msg::Var& ar) override;
        void Clear() override;
        void Resize(Vec2f size) override;

        void ImguiWorkspace(ImGui::CanvasParams& canvas) override;
        void ImguiWorkspaceMenu() override;
        void ImguiSetup() override;
        void ImguiUpdate(bool items) override;

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

    void BeginDefaultMenu(const char* id);
    bool EndDefaultMenu();

} // namespace fin
