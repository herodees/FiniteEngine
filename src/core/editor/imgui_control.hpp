#pragma once

#include "include.hpp"
#include "core/atlas.hpp"
#include "misc/cpp/imgui_stdlib.h"

namespace ImGui
{
    bool SpriteInput(const char* label, fin::Atlas::Pack* pack);
    bool SoundInput(const char* label, fin::SoundSource::Ptr* pack);
    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size, bool scene_edit = false);
    void SpriteImage(fin::Atlas::Sprite* spr, ImVec2 size);

    bool OpenFileInput(const char* label, std::string& path, const char* filter);
    bool SaveFileInput(const char* label, std::string& path, const char* filter);

    void HelpMarker(const char* desc);

    const char* FormatStr(const char* fmt, ...);

    void  SetDragData(void* d, const char* id = nullptr);
    void* GetDragData(const char* id = nullptr);

    void SetActiveVar(fin::msg::Var el);
    fin::msg::Var& GetActiveVar();

    bool FileMenu(const char* label, std::string& path, const char* filter);
    bool OpenFileName(const char* label, std::string& path, const char* filter);

} // namespace ImGui
