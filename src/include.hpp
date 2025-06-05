#pragma once
// RAYLIB
#include "raylib.h"
#include "raymath.h"

// ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include "rlImGui.h"

// ENTT
#include "entt.hpp"
namespace fin
{
    using Entity = entt::registry::entity_type;
    using Registry = entt::registry;
    using SparseSet = entt::sparse_set;

    struct EntityHandle
    {
        template <typename T>
        T& Get()
        {
            return registry->get<T>(entity);
        }
        template <typename T>
        bool Has() const
        {
            return registry->all_of<T>(entity);
        }
        template <typename T>
        void Remove()
        {
            registry->remove<T>(entity);
        }
        operator Entity() const
        {
            return entity;
        }

        Entity    entity{};
        Registry* registry{};
    };
} // namespace fin

// Utils
#include "utils/std_utils.hpp"
#include "utils/math_utils.hpp"
#include "utils/matrix2d.hpp"
#include "utils/msgvar.hpp"
