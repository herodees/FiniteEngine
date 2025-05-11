#include "scene_npc.hpp"

namespace fin
{
    std::string_view NpcSceneObject::object_type() const
    {
        return NpcSceneObject::type_id;
    }

    void NpcSceneObject::render(Renderer& dc)
    {
        SpriteSceneObject::render(dc);
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
