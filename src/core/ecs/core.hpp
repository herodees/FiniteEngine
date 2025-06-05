#pragma once

#include "system.hpp"

namespace fin
{
    class ObjectSceneLayer;
}

namespace fin::ecs
{
    void RegisterCoreSystems(SystemManager& fact);


    class Navigation : public System
    {
    public:
        Navigation(Scene& s);
        ~Navigation() override = default;

        void Update(float dt) override;
        void update_objects(float dt, ObjectSceneLayer* layer);
        bool ImguiSetup() override;
    };



    class Behavior : public System
    {
    public:
        Behavior(Scene& s);
        ~Behavior() override = default;

        void Update(float dt) override;
        void UpdateObjects(float dt, ObjectSceneLayer* layer);
        bool ImguiSetup() override;
    };



    class CameraController : public System
    {
    public:
        CameraController(Scene& s);
        ~CameraController() override = default;

        void Update(float dt) override;
        void OnStartRunning() override;
        bool ImguiSetup() override;

        void SetCamera(Entity cam);

    private:
        Entity m_Camera;
    };


} // namespace fin::ecs
