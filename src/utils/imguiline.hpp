#pragma once

#include "include.hpp"
#include <imgui_internal.h>

enum ImStyle
{
    ImStyle_InvisibleButton,
    ImStyle_Button,
    ImStyle_Frame,
    ImStyle_Header,
    ImStyle_Tab,
    ImStyle_Check,
    ImStyle_CheckAlt,
    ImStyle_CheckReadonly,
};

namespace ImGui
{
class LineConstructor;

LineConstructor& Line();
LineConstructor& LineItem(ImGuiID id, const ImVec2& size);
LineConstructor& LineSelect(ImGuiID id, bool selected);
LineConstructor& LineSelect(ImGuiID id, bool selected, const ImVec4& color);
LineConstructor& LineItem(const char* id, const ImVec2& size);
LineConstructor& LineSelect(const char* id, bool selected);
LineConstructor& LineSelect(const char* id, bool selected, const ImVec4& color);

class LineConstructor
{
    struct Block
    {
        enum Type
        {
            Text,
            Space,
            Spring,
            ColorId,
            Color,
            StyleBegin,
            StyleEnd
        };

        uint32_t type : 3;
        uint32_t hover : 1;
        uint32_t length : 12;
        uint32_t begin : 16;
        uint32_t data;
    };

public:
    bool End();

    LineConstructor& Selected(bool s);
    LineConstructor& Expandable(bool default_expand);

    template<typename STR>
    LineConstructor& Text(const STR& str);
    template<size_t N>
    LineConstructor& Text(const char (&s)[N]);
    LineConstructor& Text(const char* str);
    LineConstructor& Text(const char* str, size_t l);
    LineConstructor& Format(const char* fmt, ...);

    LineConstructor& Space(int width = 2);
    LineConstructor& Spring(float power = 1.f);

    LineConstructor& PushStyleColor(ImGuiCol col);
    LineConstructor& PopStyleColor();

    LineConstructor& PushColor(uint32_t col);
    LineConstructor& PopColor();

    LineConstructor& PushStyle(ImStyle st, int id = 0, bool selected = false);
    LineConstructor& PopStyle();

    int HoverId() const;
    bool Return() const;
    bool Expanded() const;

    ImRect GetStyleRect(int id) const;

private:
    void Precalculate();
    void Render();

    int m_spring = 0;
    ImVector<Block> m_blocks;
    ImVector<const ImFontGlyph*> m_stringGlyph;
    ImVec2 m_min;
    ImVec2 m_max;
    bool m_visible;
    bool m_hover;
    bool m_selected;
    bool m_selectable;
    bool m_return;
    int m_hoverId;
    int* m_expanded = nullptr;

    friend LineConstructor& Create(const ImVec2& from, const ImVec2& to, bool visible, bool hover, bool ret);
};

template<typename STR>
inline LineConstructor& LineConstructor::Text(const STR& str)
{
    return Text((const char*)str.data(), str.size());
}

template<size_t N>
inline LineConstructor& LineConstructor::Text(const char (&s)[N])
{
    return Text(s, N);
}

} // namespace ImGui
