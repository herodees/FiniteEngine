#pragma once

#include <imgui.h>

namespace ImGui
{
const char *FormatStr(const char *fmt, ...);

void SetDragData(void *d);
void *GetDragData();
}
