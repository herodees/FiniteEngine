
#include "utils/dialog_utils.hpp"
#include "imgui_utils.hpp"
#include "misc/cpp/imgui_stdlib.cpp"
#include "extras/IconsFontAwesome6.h"

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

}
