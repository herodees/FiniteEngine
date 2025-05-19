#pragma once

#include "include.hpp"

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
        };

        static SceneLayer* create(msg::Value& ar, Scene* scene);
        static SceneLayer* create(Type t);
        static SceneLayer* create_isometric();
        static SceneLayer* create_sprite();
        static SceneLayer* create_region();

        SceneLayer(Type t = Type::Undefined);
        virtual ~SceneLayer() = default;

        Type              type() const;
        std::string&      name();
        std::string_view& icon();
        uint32_t          color() const;
        Scene*            parent();
        const Rectf&      region() const;
        Vec2f             screen_to_view(Vec2f pos) const;
        Vec2f             view_to_screen(Vec2f pos) const;
        Vec2f             get_mouse_position() const;

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
        Type             _type;
        std::string      _name;
        std::string_view _icon;
        uint32_t         _color{0xffffffff};
        Rectf            _region;
        bool             _hidden{};
        bool             _active{};
    };

} // namespace fin
