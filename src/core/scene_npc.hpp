#pragma once

#include "scene_object.hpp"

namespace fin
{
    class NpcSceneObject : public SpriteSceneObject
    {
    public:
        inline static std::string_view type_id = "npc";

        NpcSceneObject();
        ~NpcSceneObject() override = default;

        void init() override;
        void update(float dt) override;
        void render(Renderer& dc) override;

        void edit_render(Renderer& dc, bool selected) override;
        bool imgui_update() override;

        void save(msg::Var& ar) override; // Save prefab
        void load(msg::Var& ar) override; // Load prefab

        void serialize(msg::Writer& ar) override;  // Save to scene
        void deserialize(msg::Value& ar) override; // Load to scene

        std::string_view object_type() const override;

    protected:
        std::vector<Vec2i> _path;
        size_t             _currentIndex{};
        float              _speed{132};
    };


} // namespace fin
