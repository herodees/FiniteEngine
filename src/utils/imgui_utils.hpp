#pragma once

#include <imgui.h>
#include "misc/cpp/imgui_stdlib.h"

namespace ImGui
{
bool OpenFileInput(const char *label, std::string &path, const char *filter);
bool SaveFileInput(const char *label, std::string &path, const char *filter);

const char *FormatStr(const char *fmt, ...);

void SetDragData(void *d);
void *GetDragData();
}
