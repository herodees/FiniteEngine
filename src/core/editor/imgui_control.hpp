#pragma once

#include "include.hpp"
#include "core/atlas.hpp"
#include "utils/imgui_utils.hpp"

namespace ImGui
{
    bool SpriteInput(const char* label, fin::Atlas::Pack* pack);
    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size);

} // namespace ImGui
