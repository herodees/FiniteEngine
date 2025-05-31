#pragma once

#include "system.hpp"

namespace fin
{
    class ObjectSceneLayer;
}

namespace fin::ecs
{
    void register_core_systems(SystemManager& fact);


    class Navigation : public System
    {
    public:
        Navigation(Scene& s);
        ~Navigation() override = default;

        void update(float dt) override;
        void update_objects(float dt, ObjectSceneLayer* layer);
        bool imgui_setup() override;
    };



    class Behavior : public System
    {
    public:
        Behavior(Scene& s);
        ~Behavior() override = default;

        void update(float dt) override;
        void update_objects(float dt, ObjectSceneLayer* layer);
        bool imgui_setup() override;
    };
}
