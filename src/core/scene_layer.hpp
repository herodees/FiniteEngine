#pragma once

#include "include.hpp"

namespace fin
{
    class Renderer;
    class Scene;
    struct Params;
    struct DragData;

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

        SceneLayer(Type t = Type::Undefined);
        virtual ~SceneLayer() = default;

        Type              type() const;
        std::string&      name();
        std::string_view& icon();
        uint32_t          color() const;
        Scene*            parent();
        const Recti&      region() const;

        virtual void serialize(msg::Writer& ar);
        virtual void deserialize(msg::Value& ar);
        virtual void resize(Vec2f size);
        virtual void clear();

        virtual void activate(const Recti& region);
        virtual void update(float dt);
        virtual void render(Renderer& dc);
        virtual void render_edit(Renderer& dc);

        virtual void imgui_update(bool items);
        virtual void imgui_setup();
        virtual void imgui_workspace(Params& params, DragData& drag);
        virtual void imgui_workspace_menu();

        bool is_hidden() const;
        bool is_active() const;

        void hide(bool b);
        void activate(bool a);

        void render_grid(Renderer& dc);

    protected:
        Scene*           _parent{};
        Type             _type;
        std::string      _name;
        std::string_view _icon;
        uint32_t         _color{0xffffffff};
        Recti            _region;
        bool             _hidden{};
        bool             _active{};
    };

} // namespace fin
