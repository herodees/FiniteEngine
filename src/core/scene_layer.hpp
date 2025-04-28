#pragma once

#include "include.hpp"

namespace fin
{
    class Renderer;
    class Scene;
    struct Params;
    struct DragData;

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

        static SceneLayer* create(msg::Value& ar);
        static SceneLayer* create(Type t);

        SceneLayer(Type t = Type::Undefined);
        virtual ~SceneLayer() = default;

        Type              type() const;
        std::string&      name();
        std::string_view& icon();
        Scene*            parent();

        virtual void serialize(msg::Writer& ar);
        virtual void deserialize(msg::Value& ar);
        virtual void resize(Vec2f size);

        virtual void activate(const Rectf& region);
        virtual void update(float dt);
        virtual void edit_update(Params& params, DragData& drag);
        virtual void render(Renderer& dc);

        virtual void imgui_update(bool items);

        bool is_hidden() const;
        bool is_active() const;

        void hide(bool b);
        void activate(bool a);

    private:
        Scene*           _parent{};
        Type             _type;
        std::string      _name;
        std::string_view _icon;
        bool             _hidden{};
        bool             _active{};
    };

} // namespace fin
