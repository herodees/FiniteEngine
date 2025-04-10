
#include "utils/dialog_utils.hpp"
#include "imgui_utils.hpp"
#include "misc/cpp/imgui_stdlib.cpp"
#include "extras/IconsFontAwesome6.h"
#include <raylib.h>

namespace ImGui
{
const char *FormatStr(const char *fmt, ...)
{
    static char buf[512];

    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, std::size(buf), fmt, args);
    va_end(args);

    if (w == -1 || w >= (int)std::size(buf))
        w = (int)std::size(buf) - 1;
    buf[w] = 0;
    return buf;
}

static void *s_drag_data = 0;

struct FileInfo
{
    std::string _name;
    std::vector<FileInfo> _dir;
    std::vector<std::string> _file;
    bool _expanded = false;

    bool show(std::string_view filter, std::string &selectedFile);
};

static FileInfo s_root;
static std::string s_active;
static std::string s_temp;

bool FileMenu(const char *label, std::string &path, const char *filter)
{
    if (s_active.empty())
    {
        s_active = ".";
    }
    auto ret = s_root.show(filter, path);
    if (ret)
    {
        s_root._expanded = false;
        s_active.clear();
    }
    return ret;
}

bool OpenFileName(const char *label, std::string &path, const char *filter)
{
    bool ret{};
    if (BeginCombo(label, path.c_str()))
    {
        if (ImGui::IsWindowAppearing())
        {
            s_active = ".";
            s_root._expanded = false;
        }
        ret = FileMenu(label, path, filter);
        ImGui::EndCombo();
    }
    return ret;
}

bool FileInfo::show(std::string_view filter, std::string &selectedFile)
{
    bool modified = false;

    if (!_expanded)
    {
        _dir.clear();
        _file.clear();
        _expanded = true;
        auto list = LoadDirectoryFiles(s_active.c_str());
        for (auto n = 0; n < list.count; ++n)
        {
            if (IsPathFile(list.paths[n]))
            {
                auto* ext = GetFileExtension(list.paths[n]);
                if (filter.empty() || (ext && ext == filter))
                    _file.emplace_back(GetFileName(list.paths[n]));
            }
            else
            {
                auto& dir = _dir.emplace_back();
                dir._name = GetFileName(list.paths[n]);
            }
        }
        UnloadDirectoryFiles(list);
    }

    for (auto &dir : _dir)
    {
        s_temp = ICON_FA_FOLDER " ";
        s_temp += dir._name;
        if (ImGui::BeginMenu(s_temp.c_str()))
        {
            auto _prev_size = s_active.size();
            s_active += '/';
            s_active += dir._name;
            modified |= dir.show(filter, selectedFile);
            s_active.resize(_prev_size);
            ImGui::EndMenu();
        }
    }

    for (const auto &file : _file)
    {
        if (ImGui::MenuItem(file.c_str()))
        {
            selectedFile = s_active;
            selectedFile += '/';
            selectedFile += file;
            modified = true;
        }
    }

    return modified;
}


void SetDragData(void *d)
{
    s_drag_data = d;
}

void *GetDragData()
{
    return s_drag_data;
}

void HelpMarker(const char *desc)
{
    ImGui::TextDisabled(ICON_FA_BOOK_OPEN "");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool OpenFileInput(const char *label, std::string &path, const char *filter)
{
    bool ret{};
    if (BeginCombo(label, path.c_str()))
    {
        auto r = fin::open_file_dialog("Open", path, fin::create_file_filter(filter));
        if (!r.empty())
        {
            ret = true;
            path = r[0];
        }
        ImGui::CloseCurrentPopup();
        ImGui::EndCombo();
    }
    return ret;
}

bool SaveFileInput(const char *label, std::string &path, const char *filter)
{
    bool ret{};
    if (BeginCombo(label, path.c_str()))
    {
        auto r = fin::save_file_dialog("Open", path, fin::create_file_filter(filter));
        if (!r.empty())
        {
            ret = true;
            path = r;
        }
        ImGui::CloseCurrentPopup();
        ImGui::EndCombo();
    }
    return ret;
}



} // namespace ImGui
