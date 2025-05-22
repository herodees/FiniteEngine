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
} // namespace fin

// Utils
#include "utils/std_utils.hpp"
#include "utils/math_utils.hpp"
#include "utils/matrix2d.hpp"
#include "utils/msgvar.hpp"
