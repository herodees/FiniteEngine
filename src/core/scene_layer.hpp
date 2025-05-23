#pragma once

#include "include.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class Renderer;
    class Scene;
    class IsoSceneObject;
    struct Params;
    struct DragData;

    constexpr int32_t tile_size(512);

    void BeginDefaultMenu(const char* id);
    bool EndDefaultMenu();

    class SceneLayer
    {
        friend class Scene;
    public:
        enum class Type
        {
            Undefined,
            Sprite,
            Region,
            Isometric,
            Object,
        };

        static SceneLayer* create(msg::Var& ar, Scene* scene);
        static SceneLayer* create(msg::Value& ar, Scene* scene);
        static SceneLayer* create(Type t);
        static SceneLayer* create_isometric();
        static SceneLayer* create_sprite();
        static SceneLayer* create_region();
        static SceneLayer* create_object();

        SceneLayer(Type t = Type::Undefined);
        virtual ~SceneLayer() = default;

        Type              type() const;
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

        virtual void serialize(msg::Writer& ar);
        virtual void deserialize(msg::Value& ar);
        virtual void resize(Vec2f size);
        virtual void clear();

        virtual void init();
        virtual void deinit();
        virtual void activate(const Rectf& region);
        virtual void update(float dt);
        virtual void render(Renderer& dc);
        virtual void render_edit(Renderer& dc);

        virtual std::span<const Vec2i> find_path(const IsoSceneObject* obj, Vec2i target);
        virtual void moveto(IsoSceneObject* obj, Vec2f pos);

        virtual void imgui_update(bool items);
        virtual void imgui_setup();
        virtual void imgui_workspace(Params& params, DragData& drag);
        virtual void imgui_workspace_menu();

        bool is_hidden() const;
        bool is_active() const;

        void hide(bool b);
        void activate(bool a);

    protected:
        Scene*           _parent{};
        int32_t          _index{-1};
        Type             _type;
        std::string      _name;
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

        void serialize(msg::Var& ar) override;
        void deserialize(msg::Var& ar) override;

        void clear() override;
        void resize(Vec2f size) override;
        void activate(const Rectf& region) override;

        void   insert(Entity ent);
        void   remove(Entity ent);
        void   moveto(Entity ent, Vec2f pos);
        void   move(Entity ent, Vec2f pos);
        Entity find_at(Vec2f position) const;
        Entity find_active_at(Vec2f position) const;
        void   select_edit(Entity ent);

        void update(float dt) override;
        void render(Renderer& dc) override;
        void render_edit(Renderer& dc) override;

        void imgui_workspace(Params& params, DragData& drag) override;
        void imgui_workspace_menu() override;
        void imgui_update(bool items) override;

    protected:
        entt::sparse_set        _objects;
        entt::sparse_set        _selected;
        Vec2i                   _grid_size;
        lq::SpatialDatabase     _spatial_db;
        std::vector<IsoObject>  _iso_pool;
        std::vector<IsoObject*> _iso;
        uint32_t                _iso_pool_size{};
        int32_t                 _inflate{};
        Entity                  _edit{entt::null};
        Entity                  _drop{entt::null};
    };

} // namespace fin
