#pragma once

#include "include.hpp"
#include "utils/lquery.hpp"
#include "navmesh.hpp"

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class Renderer;
    class Scene;
    class IsoSceneObject;
    struct Params;
    struct DragData;

    constexpr int32_t tile_size(512);

    namespace LayerType
    {
        constexpr std::string_view Object = "obj";
        constexpr std::string_view Sprite = "spr";
        constexpr std::string_view Region = "reg";
    } // namespace LayerType

    class SceneLayer
    {
        friend class Scene;
    public:
        static SceneLayer* create(msg::Var& ar, Scene* scene);
        static SceneLayer* create(std::string_view t);

        static SceneLayer* create_sprite();
        static SceneLayer* create_region();
        static SceneLayer* create_object();

        SceneLayer(std::string_view t);
        virtual ~SceneLayer() = default;

        std::string_view  type() const;
        std::string&      name();
        std::string_view& icon();
        uint32_t          color() const;
        int32_t           index() const;
        Scene*            parent();
        const Rectf&      region() const;
        Vec2f             screen_to_view(Vec2f pos) const;
        Vec2f             view_to_screen(Vec2f pos) const;
        Vec2f             get_mouse_position() const;

        virtual void serialize(msg::Var& ar);
        virtual void deserialize(msg::Var& ar);
        virtual void resize(Vec2f size);
        virtual void clear();
        virtual void init();
        virtual void deinit();
        virtual void activate(const Rectf& region);
        virtual void update(float dt);
        virtual void fixed_update(float dt);
        virtual void render(Renderer& dc);
        virtual void imgui_update(bool items);
        virtual void imgui_setup();
        virtual void imgui_workspace(Params& params, DragData& drag);
        virtual void imgui_workspace(ImGui::CanvasParams& canvas);
        virtual void imgui_workspace_menu();

        bool is_hidden() const;
        bool is_active() const;

        void hide(bool b);
        void activate(bool a);

    protected:
        Scene*           _parent{};
        int32_t          _index{-1};
        std::string      _name;
        std::string      _type;
        std::string_view _icon;
        uint32_t         _color{0xffffffff};
        Rectf            _region;
        bool             _hidden{};
        bool             _active{};
    };



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

        void   serialize(msg::Var& ar) override;
        void   deserialize(msg::Var& ar) override;
        void   clear() override;
        void   resize(Vec2f size) override;
        void   activate(const Rectf& region) override;
        void   insert(Entity ent);
        void   remove(Entity ent);
        void   moveto(Entity ent, Vec2f pos);
        void   move(Entity ent, Vec2f pos);
        void   update(void* obj);
        Entity find_at(Vec2f position) const;
        Entity find_active_at(Vec2f position) const;
        bool   find_path(Vec2i from, Vec2i to, std::vector<Vec2i>& path) const;
        void   select_edit(Entity ent);
        void   update(float dt) override;
        void   render(Renderer& dc) override;
        void   imgui_workspace(ImGui::CanvasParams& canvas) override;
        void   imgui_workspace_menu() override;
        void   imgui_setup() override;
        void   imgui_update(bool items) override;

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
