#pragma once

#include "include.hpp"
#include "core/atlas.hpp"
#include "misc/cpp/imgui_stdlib.h"

namespace ImGui
{
    void ScrollWhenDragging(const ImVec2& aDeltaMult, ImGuiMouseButton aMouseButton, ImGuiID testid);

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

    class Editor : std::enable_shared_from_this<Editor>
    {
    public:
        static std::shared_ptr<Editor> load_from_file(std::string_view path);

        Editor()          = default;
        virtual ~Editor() = default;

        virtual bool load(std::string_view path) = 0;
        virtual bool imgui_show() = 0;
    };

} // namespace ImGui
