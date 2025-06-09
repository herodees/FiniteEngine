#pragma once

#include "types.hpp"
#include <algorithm>

namespace fin
{
    extern GameAPI gGameAPI;

    class ScriptComponent
    {
    public:
        ScriptComponent()                   = default;
        virtual ~ScriptComponent()          = default;
        virtual void Init(Entity entity)     = 0; // Initialize the component for the given entity
        virtual void Update(float deltaTime) = 0; // Update the component logic
    };

    template <typename T>
    struct ScriptTraits
    {
        static ComponentId              component_id; // Unique ID for the component type
        static entt::storage<T>*        storage;
        static ComponentInfoStorage<T>* info;
    };

    template <typename T>
    ComponentId ScriptTraits<T>::component_id{};
    template <typename T>
    entt::storage<T>* ScriptTraits<T>::storage{};
    template <typename T>
    ComponentInfoStorage<T>* ScriptTraits<T>::info{};

} // namespace fin
