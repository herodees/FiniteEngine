#include "imguiline.hpp"

namespace ImGui
{
static LineConstructor g_line;

static LineConstructor& Create(const ImVec2& from, const ImVec2& to, bool visible, bool hover, bool ret)
{
    g_line.m_blocks.resize(0);
    g_line.m_stringGlyph.resize(0);
    g_line.m_spring = 0;
    g_line.m_min = from;
    g_line.m_max = to;
    g_line.m_visible = visible;
    g_line.m_hover = hover;
    g_line.m_return = ret;
    g_line.m_hoverId = 0;
    g_line.m_selected = false;
    g_line.m_selectable = false;

    return g_line;
}

LineConstructor& ImGui::Line()
{
    return g_line;
}

LineConstructor& LineItem(ImGuiID id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    // Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
    IM_ASSERT(size_arg.x != 0.0f && size_arg.y != 0.0f);
    ImVec2 size = CalcItemSize(size_arg, 0.0f, 0.0f);
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    ItemSize(size);
    const bool hidden = (!ItemAdd(bb, id)) || window->SkipItems || !ImGui::IsItemVisible();
    bool hovered, held;
    const bool r = ButtonBehavior(bb, id, &hovered, &held, 0);
    return Create(GImGui->LastItemData.Rect.Min, GImGui->LastItemData.Rect.Max, !hidden, ImGui::IsItemHovered(), r);
}

LineConstructor& LineSelect(ImGuiID id, bool selected)
{
    ImGui::PushID(id);
    const bool r = ImGui::Selectable("###-", selected);
    ImGui::PopID();
    return Create(GImGui->LastItemData.Rect.Min, GImGui->LastItemData.Rect.Max, ImGui::IsItemVisible(), ImGui::IsItemHovered(), r);
}

LineConstructor& LineSelect(ImGuiID id, bool selected, const ImVec4& color)
{
    if (color.w == 0 && color.x == 0 && color.y == 0 && color.z == 0)
        return LineSelect(id, selected);

    ImVec4 old = ImGui::GetStyle().Colors[ImGuiCol_Header];
    ImGui::GetStyle().Colors[ImGuiCol_Header] = color;
    if (selected)
        ImGui::GetStyle().Colors[ImGuiCol_Header] = old;
    ImGui::PushID(id);
    const bool r = ImGui::Selectable("###-", true);
    ImGui::PopID();
    ImGui::GetStyle().Colors[ImGuiCol_Header] = old;

    return Create(GImGui->LastItemData.Rect.Min, GImGui->LastItemData.Rect.Max, ImGui::IsItemVisible(), ImGui::IsItemHovered(), r);
}

LineConstructor& LineItem(const char* id, const ImVec2& size)
{
    return LineItem(GetID(id), size);
}

LineConstructor& LineSelect(const char* id, bool selected)
{
    return LineSelect(GetID(id), selected);
}

LineConstructor& LineSelect(const char* id, bool selected, const ImVec4& color)
{
    return LineSelect(GetID(id), selected, color);
}

int LineHover()
{
    return g_line.HoverId();
}

bool LineConstructor::End()
{
    if (!m_visible)
        return m_return;

    Precalculate();
    Render();

    return m_return;
}

LineConstructor& LineConstructor::Selected(bool s)
{
    m_selected = s;
    m_selectable = true;
    return *this;
}

LineConstructor& LineConstructor::Text(const char* str)
{
    return Text(str, -1);
}

LineConstructor& LineConstructor::Text(const char* str, size_t len)
{
    if (!m_visible || !len)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::Text;
    block.begin = (uint16_t)m_stringGlyph.Size;
    if (len == -1)
        len = strlen(str);

    ImFont* font = ImGui::GetFont();
    const char* current = str;
    const char* cend = str + len;
    uint32_t c = 0;
    while (current < cend)
    {
        current += ImTextCharFromUtf8(&c, current, cend);
        if (c == '\0')
            break;
        if (c == '\r' || c == '\n')
            continue;

        const ImFontGlyph* glyph = font->FindGlyph((unsigned short)c);

        m_stringGlyph.push_back(glyph);
    }

    block.length = (uint16_t)m_stringGlyph.Size - block.begin;

    return *this;
}

LineConstructor& LineConstructor::Format(const char* fmt, ...)
{
    if (!m_visible)
        return *this;
    static char buf[512];
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, (int)std::size(buf), fmt, args);
    va_end(args);
    if (w == -1 || w >= (int)std::size(buf))
        w = (int)std::size(buf) - 1;
    buf[w] = 0;

    return Text(buf, w);
}

LineConstructor& LineConstructor::Space(int width)
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::Space;
    block.length = (uint32_t)(width * ImGui::GetWindowDpiScale());
    block.begin = 0;
    block.data = 0;
    return *this;
}

LineConstructor& LineConstructor::Spring(float power)
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::Spring;
    block.length = 0;
    block.begin = 0;
    block.data = *(uint32_t*)&power;
    ++m_spring;
    return *this;
}

LineConstructor& LineConstructor::PushStyleColor(ImGuiCol col)
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::ColorId;
    block.begin = 1;
    block.data = col;
    return *this;
}

LineConstructor& LineConstructor::PopStyleColor()
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::ColorId;
    block.begin = 0;
    return *this;
}

LineConstructor& LineConstructor::PushColor(uint32_t col)
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::Color;
    block.begin = 1;
    block.data = col;
    return *this;
}

LineConstructor& LineConstructor::PopColor()
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::Color;
    block.begin = 0;
    return *this;
}

LineConstructor& LineConstructor::PushStyle(ImStyle st, int id, bool selected)
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::StyleBegin;
    block.hover = selected;
    block.length = st;
    block.data = id;
    return *this;
}

LineConstructor& LineConstructor::PopStyle()
{
    if (!m_visible)
        return *this;
    m_blocks.resize(m_blocks.Size + 1);
    Block& block = m_blocks.back();
    block.type = Block::Type::StyleEnd;
    return *this;
}

int LineConstructor::HoverId() const
{
    return m_hoverId;
}

bool LineConstructor::Return() const
{
    return m_return;
}

ImRect LineConstructor::GetStyleRect(int id) const
{
    for (int n = 0; n < m_blocks.Size; ++n)
    {
        const Block& current = m_blocks[n];
        if (current.type == Block::Type::StyleBegin && current.data == id)
        {
            ImRect rc(GImGui->LastItemData.Rect);
            rc.Min.x += current.length;
            rc.Max.x = rc.Min.x + current.begin;
            return rc;
        }
    }
    return ImRect();
}

void LineConstructor::Precalculate()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float scale = ImGui::GetWindowDpiScale();
    float spring_width = 0;

    Block root = {};
    Block* block_stack[6];
    block_stack[0] = &root;
    int stack_idx = 0;

    for (int n = 0; n < m_blocks.Size; ++n)
    {
        Block& current = m_blocks[n];

        if (current.type == Block::Type::Text)
        {
            for (unsigned int i = current.begin; i < current.begin + current.length; ++i)
                if (m_stringGlyph[i])
                    block_stack[stack_idx]->begin += (int)(m_stringGlyph[i]->AdvanceX * scale + 0.5f);
        }
        else if (current.type == Block::Type::Space)
        {
            block_stack[stack_idx]->begin += current.length;
        }
        else if (current.type == Block::Type::Spring)
        {
            spring_width += *(float*)&current.data;
        }
        else if (current.type == Block::Type::StyleBegin)
        {
            ++stack_idx;
            block_stack[stack_idx] = &current;
            current.begin = (current.length == ImStyle_Check || current.length == ImStyle_CheckAlt || current.length == ImStyle_CheckReadonly) ? (int)ImGui::GetFontSize() + (int)style.FramePadding.x : (int)style.FramePadding.x;
            assert(stack_idx < (int)std::size(block_stack)); // Style stack owerflow
        }
        else if (current.type == Block::Type::StyleEnd)
        {
            block_stack[stack_idx]->begin += (int)style.FramePadding.x;
            block_stack[stack_idx - 1]->begin += block_stack[stack_idx]->begin;
            --stack_idx;
            assert(stack_idx >= 0); // Style stack owerflow
        }
    }

    float free_size = (m_max.x - m_min.x) - root.begin;

    if (m_spring && free_size > 0)
    {
        float mul = free_size / spring_width;
        for (int n = 0; n < m_blocks.Size; ++n)
        {
            Block& current = m_blocks[n];
            if (current.type == Block::Type::Spring)
            {
                current.begin = (int)(*(float*)&current.data * mul);
                block_stack[stack_idx]->begin += current.begin;
            }
            else if (current.type == Block::Type::StyleBegin)
            {
                ++stack_idx;
                block_stack[stack_idx] = &current;
                assert(stack_idx < (int)std::size(block_stack)); // Style stack owerflow
            }
            else if (current.type == Block::Type::StyleEnd)
            {
                block_stack[stack_idx - 1]->begin += block_stack[stack_idx]->begin;
                --stack_idx;
                assert(stack_idx >= 0); // Style stack owerflow
            }
        }
    }

    assert(stack_idx == 0); // Missing PopStyle!!!
}

void LineConstructor::Render()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();

    ImVec2 mpos = ImGui::GetMousePos();
    float xpos = m_min.x;
    float ypos = m_min.y;
    float height = m_max.y - m_min.y;

    uint32_t color[6];
    color[0] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    int color_idx = 0;

    if (m_selectable)
    {
        if (m_selected)
        {
            draw_list->AddRectFilled(m_min, m_max, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Header]));
        }
        else if (m_hover)
        {
            draw_list->AddRectFilled(m_min, m_max, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_HeaderHovered]));
        }
    }

    const bool active = m_hover && ImGui::IsMouseDown(0);

    for (int n = 0; n < m_blocks.Size; ++n)
    {
        Block& current = m_blocks[n];

        if (current.type == Block::Type::Text)
        {
            //draw_list->AddText(ImVec2(xpos, m_min.y), clr, "", 0);
            const float scale = ImGui::GetWindowDpiScale();
            float x = xpos;
            float y = ypos + int((height - ImGui::GetFont()->FontSize * scale) * 0.5f);
            const int vtx_count_max = (int)(current.length) * 4;
            const int idx_count_max = (int)(current.length) * 6;
            const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
            draw_list->PrimReserve(idx_count_max, vtx_count_max);

            ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
            ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
            unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

            for (uint32_t s = current.begin; s < current.begin + current.length; ++s)
            {
                const ImFontGlyph* glyph = m_stringGlyph[s];
                if (!glyph)
                    continue;
                
                float char_width = (float)(int)(glyph->AdvanceX * scale + 0.5f);
                if (glyph->Visible)
                {
                    // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
                    float x1 = x + glyph->X0 * scale;
                    float x2 = x + glyph->X1 * scale;
                    float y1 = y + glyph->Y0 * scale;
                    float y2 = y + glyph->Y1 * scale;

                    // Render a character
                    float u1 = glyph->U0;
                    float v1 = glyph->V0;
                    float u2 = glyph->U1;
                    float v2 = glyph->V1;

                    // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                    {
                        idx_write[0] = (ImDrawIdx)(vtx_current_idx);
                        idx_write[1] = (ImDrawIdx)(vtx_current_idx + 1);
                        idx_write[2] = (ImDrawIdx)(vtx_current_idx + 2);
                        idx_write[3] = (ImDrawIdx)(vtx_current_idx);
                        idx_write[4] = (ImDrawIdx)(vtx_current_idx + 2);
                        idx_write[5] = (ImDrawIdx)(vtx_current_idx + 3);
                        vtx_write[0].pos.x = x1;
                        vtx_write[0].pos.y = y1;
                        vtx_write[0].col = color[color_idx];
                        vtx_write[0].uv.x = u1;
                        vtx_write[0].uv.y = v1;
                        vtx_write[1].pos.x = x2;
                        vtx_write[1].pos.y = y1;
                        vtx_write[1].col = color[color_idx];
                        vtx_write[1].uv.x = u2;
                        vtx_write[1].uv.y = v1;
                        vtx_write[2].pos.x = x2;
                        vtx_write[2].pos.y = y2;
                        vtx_write[2].col = color[color_idx];
                        vtx_write[2].uv.x = u2;
                        vtx_write[2].uv.y = v2;
                        vtx_write[3].pos.x = x1;
                        vtx_write[3].pos.y = y2;
                        vtx_write[3].col = color[color_idx];
                        vtx_write[3].uv.x = u1;
                        vtx_write[3].uv.y = v2;
                        vtx_write += 4;
                        vtx_current_idx += 4;
                        idx_write += 6;
                    }
                }

                x += char_width;
            }
            xpos = x;
            // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
            draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
            draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
            draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
            draw_list->_VtxWritePtr = vtx_write;
            draw_list->_IdxWritePtr = idx_write;
            draw_list->_VtxCurrentIdx = vtx_current_idx;
        }
        else if (current.type == Block::Type::Space)
        {
            xpos += current.length;
        }
        else if (current.type == Block::Type::Spring)
        {
            xpos += current.begin;
        }
        else if (current.type == Block::Type::StyleBegin)
        {
            const bool hovered = (m_hover && current.data && mpos.x >= xpos && mpos.x < xpos + current.begin);
            if (hovered)
                m_hoverId = current.data;

            uint32_t clr = 0;
            if (current.length == ImStyle_Button)
                clr = ImGui::ColorConvertFloat4ToU32(style.Colors[current.hover || hovered ? (active ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_Button]);
            else if (current.length == ImStyle_Frame)
                clr = ImGui::ColorConvertFloat4ToU32(style.Colors[current.hover || hovered ? (active ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBgHovered) : ImGuiCol_FrameBg]);
            else if (current.length == ImStyle_Header)
                clr = ImGui::ColorConvertFloat4ToU32(style.Colors[current.hover || hovered ? (active ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered) : ImGuiCol_Header]);
            else if (current.length == ImStyle_InvisibleButton)
                clr = 0;
            else if (current.length == ImStyle_Check || current.length == ImStyle_CheckAlt || current.length == ImStyle_CheckReadonly)
            {
                float sqr_size = ImGui::GetFrameHeight()-2;
                float sqr_shift = (height - sqr_size) / 2;
                draw_list->AddRectFilled(ImVec2(xpos, m_min.y + sqr_shift), ImVec2(xpos + sqr_size, m_min.y + sqr_shift + sqr_size),
                                         ImGui::ColorConvertFloat4ToU32(style.Colors[hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg]),
                                         style.FrameRounding);
                if (current.hover)
                {
                    const float pad = std::max(1.0f, floorf(sqr_size / 6.0f));
                    if (current.length == ImStyle_CheckAlt)
                        draw_list->AddRectFilled(ImVec2(xpos + pad, m_min.y + sqr_shift + pad), ImVec2(xpos + sqr_size - pad, m_min.y + sqr_shift + sqr_size - pad),
                                                 GetColorU32(ImGuiCol_CheckMark),
                                                 style.FrameRounding);
                    else if (current.length == ImStyle_Check)
                        RenderCheckMark(draw_list, ImVec2(xpos + pad, m_min.y + sqr_shift + pad), GetColorU32(ImGuiCol_CheckMark), sqr_size - pad * 2.0f);
                    else 
                        RenderCheckMark(draw_list, ImVec2(xpos + pad, m_min.y + sqr_shift + pad), GetColorU32(ImGuiCol_TextDisabled), sqr_size - pad * 2.0f);
                }
                xpos += sqr_size;
                clr = 0;
            }

            if (clr & 0xff000000)
            {
                current.length = uint32_t(xpos - m_min.x);
                ImGui::RenderFrame(ImVec2(xpos, m_min.y), ImVec2(xpos + current.begin, m_max.y),
                                   clr, style.FrameBorderSize, style.FrameRounding);
            }
            xpos += style.FramePadding.x;
        }
        else if (current.type == Block::Type::StyleEnd)
        {
            xpos += style.FramePadding.x;
        }
        else if (current.type == Block::Type::ColorId)
        {
            if (current.begin)
                color[++color_idx] = ImGui::ColorConvertFloat4ToU32(style.Colors[current.data]);
            else
                --color_idx;
            assert(color_idx >= 0 && color_idx < (int)std::size(color)); // Missing PopStyleColor
        }
        else if (current.type == Block::Type::Color)
        {
            if (current.begin)
                color[++color_idx] = current.data;
            else
                --color_idx;
            assert(color_idx >= 0 && color_idx < (int)std::size(color)); // Missing PopColor
        }
    }
}
} // namespace ImGui
