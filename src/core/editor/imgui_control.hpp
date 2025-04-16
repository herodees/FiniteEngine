#pragma once

#include "include.hpp"
#include "core/atlas.hpp"
#include "misc/cpp/imgui_stdlib.h"

namespace ImGui
{
    bool SpriteInput(const char* label, fin::Atlas::Pack* pack);
    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size);
    void SpriteImage(fin::Atlas::Sprite* spr, ImVec2 size);

    bool OpenFileInput(const char* label, std::string& path, const char* filter);
    bool SaveFileInput(const char* label, std::string& path, const char* filter);

    void HelpMarker(const char* desc);

    const char* FormatStr(const char* fmt, ...);

    void  SetDragData(void* d);
    void* GetDragData();

    bool FileMenu(const char* label, std::string& path, const char* filter);
    bool OpenFileName(const char* label, std::string& path, const char* filter);

} // namespace ImGui
