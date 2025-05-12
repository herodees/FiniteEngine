#include "scene_npc.hpp"
#include "scene_layer.hpp"
#include "renderer.hpp"

namespace fin
{
    std::string_view NpcSceneObject::object_type() const
    {
        return NpcSceneObject::type_id;
    }

    NpcSceneObject::NpcSceneObject()
    {
    }

    void NpcSceneObject::update(float dt)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            auto mpos = _layer->get_mouse_position();
            _path     = find_path(mpos);
            _currentIndex = 0;
        }

        if (_currentIndex >= _path.size())
            return;

        Vec2f target    = _path[_currentIndex];
        Vec2f direction = (target - _position).normalized();
        Vec2f movement  = direction * _speed * dt;

        if ((_position - target).length() <= movement.length())
        {
            move_to(target);
            _currentIndex++;
        }
        else
        {
            move_to(_position + movement);
        }
    }

    void NpcSceneObject::render(Renderer& dc)
    {
        SpriteSceneObject::render(dc);

        if (_path.size())
        {
            dc.set_color(RED);
            for (uint32_t n = 0; n < _path.size() - 1; ++n)
            {
                dc.render_line(_path[n], _path[n + 1]);
            }
            dc.set_color(WHITE);
        }
    }

    void NpcSceneObject::edit_render(Renderer& dc, bool selected)
    {
        SpriteSceneObject::edit_render(dc, selected);
    }

    bool NpcSceneObject::imgui_update()
    {
        auto ret = SpriteSceneObject::imgui_update();

        return ret;
    }

    void NpcSceneObject::save(msg::Var& ar)
    {
        SpriteSceneObject::save(ar);
    }

    void NpcSceneObject::load(msg::Var& ar)
    {
        SpriteSceneObject::load(ar);
    }

    void NpcSceneObject::serialize(msg::Writer& ar)
    {
        SpriteSceneObject::serialize(ar);
    }

    void NpcSceneObject::deserialize(msg::Value& ar)
    {
        SpriteSceneObject::deserialize(ar);
    }

} // namespace fin
