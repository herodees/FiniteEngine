#pragma once

#include "include.hpp"

namespace fin
{
    class Renderer;

    class SceneLayer
    {
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

        virtual void serialize(msg::Writer& ar);
        virtual void deserialize(msg::Value& ar);
        virtual void resize(Vec2f size);

        virtual void activate(const Rectf& region);
        virtual void update(float dt);
        virtual void render(Renderer& dc);

        bool is_hidden() const;
        bool is_active() const;
        bool is_edit() const;

        void hide(bool b);
        void activate(bool a);
        void edit(bool a);

    private:
        Type             _type;
        std::string      _name;
        std::string_view _icon;
        bool             _edit{};
        bool             _hidden{};
        bool             _active{};
    };

} // namespace fin
