#include "imgui_control.hpp"
#include "extras/IconsFontAwesome6.h"
#include "misc/cpp/imgui_stdlib.cpp"
#include "utils/dialog_utils.hpp"
#include "utils/imguiline.hpp"
#include <raylib.h>
#include <imgui_internal.h>


namespace ImGui
{
    struct SharedData
    {
        void*         drag_data = 0;
        std::string   drag_type;
        std::string   buff;
        fin::msg::Var selected_var;
    } s_shared;



    void ScrollWhenDragging(const ImVec2& aDeltaMult, ImGuiMouseButton aMouseButton, ImGuiID testid)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();

        if (g.MovingWindow != nullptr)
        {
            return;
        }

        ImGuiWindow* window = g.CurrentWindow;
        if (!window->ScrollbarX && !window->ScrollbarY) // Nothing to scroll
        {
            return;
        }

        ImGuiIO& im_io = ImGui::GetIO();

        bool hovered = false;
        bool held    = false;

        const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
        bool               window_highlight    = (window_to_highlight &&
                                 (window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight ||
                                  (window->DockNode && window->DockNode == window_to_highlight->DockNode)));

        ImGuiButtonFlags button_flags = (aMouseButton == 0)   ? ImGuiButtonFlags_MouseButtonLeft
                                        : (aMouseButton == 1) ? ImGuiButtonFlags_MouseButtonRight
                                                              : ImGuiButtonFlags_MouseButtonMiddle;
        if (g.HoveredId == testid            // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
            && im_io.MouseDown[aMouseButton] // Mouse pressed
            && window_highlight              // Window active
        )
        {
            ImGui::ButtonBehavior(window->InnerClipRect, window->GetID("##scrolldraggingoverlay"), &hovered, &held, button_flags);

            if ((window->InnerClipRect.Contains(im_io.MousePos)))
            {
                held = true;
            }
            else if (window->InnerClipRect.Contains(
                         im_io.MouseClickedPos[aMouseButton])) // If mouse has moved outside window, check if click was inside
            {
                held = true;
            }
            else
            {
                held = false;
            }
        }

        if (held && aDeltaMult.x != 0.0f)
            ImGui::SetScrollX(window, window->Scroll.x + aDeltaMult.x * im_io.MouseDelta.x);
        if (held && aDeltaMult.y != 0.0f)
            ImGui::SetScrollY(window, window->Scroll.y + aDeltaMult.y * im_io.MouseDelta.y);
    }

    bool SpriteInput(const char* label, fin::Atlas::Pack* pack)
    {
        ImGui::PushID(ImGui::GetID(label));
        bool modified = false;

        ImGui::BeginGroup();
        {
            if (pack->sprite)
            {
                auto& sp = *pack->sprite;
                ImGui::Image((ImTextureID)sp._texture,
                             {32, 32},
                             {sp._source.x / sp._texture->width, sp._source.y / sp._texture->height},
                             {sp._source.x2() / sp._texture->width, sp._source.y2() / sp._texture->height});
                ImGui::SameLine();
            }
            else
            {
                ImGui::InvisibleButton("##img", {32, 32});
            }

            if (pack->atlas)
            {
                s_shared.buff = pack->atlas->get_path();
            }
            else
            {
                s_shared.buff.clear();
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                if (ImGui::OpenFileName(label, s_shared.buff, ".atlas"))
                {
                    pack->atlas  = fin::Atlas::load_shared(s_shared.buff);
                    pack->sprite = nullptr;
                    modified     = true;
                }

                if (pack->atlas)
                {
                    if (ImGui::BeginCombo("##sprid", pack->sprite ? pack->sprite->_name.c_str() : ""))
                    {
                        for (auto n = 0; n < pack->atlas->size(); ++n)
                        {
                            auto& sp = pack->atlas->get(n + 1);
                            ImGui::Image((ImTextureID)sp._texture,
                                         {24, 24},
                                         {sp._source.x / sp._texture->width, sp._source.y / sp._texture->height},
                                         {sp._source.x2() / sp._texture->width, sp._source.y2() / sp._texture->height});
                            ImGui::SameLine();
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 16});
                            if (ImGui::Selectable(sp._name.c_str(), &sp == pack->sprite))
                            {
                                pack->sprite = &sp;
                                modified = true;
                            }
                            ImGui::PopStyleVar();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_TRASH))
                    {
                        pack->sprite = nullptr;
                        pack->atlas.reset();
                    }
                }
                else
                {
                    if (ImGui::BeginCombo("##sprid", ""))
                    {
                        ImGui::EndCombo();
                    }
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();
        ImGui::PopID();

        return modified;
    }

    bool TextureInput(const char* label, fin::Texture2D::Ptr* pack)
    {
        ImGui::PushID(ImGui::GetID(label));
        bool modified = false;

        ImGui::BeginGroup();
        {
            fin::Texture2D::Ptr& txt = *pack;

            if (txt)
            {
                ImGui::Image((ImTextureID)txt->get_texture(), {32, 32});
                ImGui::SameLine();
            }
            else
            {
                ImGui::InvisibleButton("##img", {32, 32});
            }

            if (txt)
            {
                s_shared.buff = txt->get_path();
            }
            else
            {
                s_shared.buff.clear();
            }

            ImGui::SameLine();

            if (ImGui::OpenFileName(label, s_shared.buff, ".png"))
            {
                txt      = fin::Texture2D::load_shared(s_shared.buff);
                modified = true;
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();

        return modified;
    }

    bool SoundInput(const char* label, fin::SoundSource::Ptr* pack)
    {
        ImGui::PushID(ImGui::GetID(label));
        bool modified = false;

        ImGui::BeginGroup();
        {
            if (pack->get())
            {
                if (ImGui::Button(ICON_FA_PLAY "##img", { 32, 32 }))
                {
                    PlaySound(*pack->get()->get_sound());
                }
            }
            else
            {
                ImGui::Button(ICON_FA_MUSIC "##img", {32, 32});
            }

            if (pack->get())
            {
                s_shared.buff = pack->get()->get_path();
            }
            else
            {
                s_shared.buff.clear();
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                if (ImGui::OpenFileName(label, s_shared.buff, ".ogg"))
                {
                    *pack         = fin::SoundSource::load_shared(s_shared.buff);
                    modified     = true;
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();
        ImGui::PopID();

        return modified;
    }

    bool PointVector(const char* label, std::vector<fin::Vec2f>* points, ImVec2 size)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));

        ImGui::LabelText(label, "");
        if (ImGui::BeginChildFrame(ImGui::GetID("pts"), size))
        {
            for (auto n = 0; n < points->size(); ++n)
            {
                ImGui::PushID(n);
                if (ImGui::Button("-"))
                {
                    points->erase(points->begin()+n);
                    ImGui::PopID();
                    continue;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputFloat2("_", &(*points)[n].x))
                {
                    modified = true;
                }
                ImGui::PopID();
            }

            if (ImGui::Button("+"))
            {
                points->emplace_back();
            }
        }
        ImGui::EndChildFrame();

        ImGui::PopID();
        return modified;
    }

    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size, bool scene_edit)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));

        ImGui::Text(label);
        if (ImGui::BeginChildFrame(ImGui::GetID("pts"), size))
        {
            for (auto n = 0; n < points->size(); n += 2)
            {
                ImGui::PushID(n);
                if (ImGui::Button("-"))
                {
                    points->erase(n);
                    points->erase(n);
                    ImGui::PopID();
                    continue;
                }
                ImGui::SameLine();
                float val[] = {points->get_item(n).get(0.0f), points->get_item(n + 1).get(0.0f)};
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputFloat2("_", val))
                {
                    modified = true;
                    points->set_item(n, val[0]);
                    points->set_item(n + 1, val[1]);
                }
                ImGui::PopID();
            }

            if (ImGui::Button("+"))
            {
                points->push_back(0.f);
                points->push_back(0.f);
            }
        }
        ImGui::EndChildFrame();

        ImGui::PopID();
        return modified;
    }

    void SpriteImage(fin::Atlas::Sprite* spr, ImVec2 size)
    {
        ImGui::Image((ImTextureID)spr->_texture,
                     size,
                     {spr->_source.x / spr->_texture->width, spr->_source.y / spr->_texture->height},
                     {spr->_source.x2() / spr->_texture->width, spr->_source.y2() / spr->_texture->height});
    }


    const char* FormatStr(const char* fmt, ...)
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



    struct FileInfo
    {
        std::string              _name;
        std::vector<FileInfo>    _dir;
        std::vector<std::string> _file;
        bool                     _expanded = false;

        bool show(std::string_view filter, std::string& selectedFile);
    };

    static FileInfo    s_root;
    static std::string s_active;
    static std::string s_temp;

    bool FileMenu(std::string& path, const char* filter)
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

    bool OpenFileName(const char* label, std::string& path, const char* filter)
    {
        bool ret{};
        if (BeginCombo(label, path.c_str()))
        {
            if (ImGui::IsWindowAppearing())
            {
                s_active         = ".";
                s_root._expanded = false;
            }
            ret = FileMenu( path, filter);
            ImGui::EndCombo();
        }
        return ret;
    }

    bool FileInfo::show(std::string_view filter, std::string& selectedFile)
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

        for (auto& dir : _dir)
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

        for (const auto& file : _file)
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


    void SetDragData(void* d, const char* id)
    {
        s_shared.drag_type = id ? id : "";
        s_shared.drag_data = d;
    }

    void* GetDragData(const char* id)
    {
        if (!id || s_shared.drag_type == id)
            return s_shared.drag_data;
        return nullptr;
    }

    void SetActiveVar(fin::msg::Var el)
    {
        s_shared.selected_var = el;
    }

    fin::msg::Var& GetActiveVar()
    {
        return s_shared.selected_var;
    }

    void HelpMarker(const char* desc)
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

    bool BeginCanvas(const char* id, ImVec2 size, CanvasParams& params)
    {
        // Start child window
        if (!ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
            return false;

        ImVec2 canvas_pos  = ImGui::GetCursorScreenPos();    // top-left
        ImVec2 canvas_size = ImGui::GetContentRegionAvail(); // current visible size

        ImGui::InvisibleButton("canvas_bg", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();

        // Handle scrolling (pan with right click drag)
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            params.origin.x += ImGui::GetIO().MouseDelta.x;
            params.origin.y += ImGui::GetIO().MouseDelta.y;
        }

        // Handle zoom with mouse wheel (optional: zoom to cursor)
        if (hovered)
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                ImVec2 mouse  = ImGui::GetIO().MousePos;
                ImVec2 before = (mouse - canvas_pos - params.origin) / params.zoom;
                params.zoom *= (1.0f + wheel * 0.1f);
                params.zoom   = ImClamp(params.zoom, params.min_zoom, params.max_zoom);
                ImVec2 after  = (mouse - canvas_pos - params.origin) / params.zoom;
                ImVec2 offset = (after - before) * params.zoom;
                params.origin += offset; // Clamp zoom

            }
        }

        // Setup canvas state
        params.canvas_pos        = canvas_pos;
        params.canvas_size       = canvas_size;
        params.mouse_pos_canvas  = ImGui::GetIO().MousePos - canvas_pos;
        params.mouse_pos_world   = (params.mouse_pos_canvas - params.origin) / params.zoom;
        params.mouse_pos_world.x = roundf(params.mouse_pos_world.x / params.snap_grid.x) * params.snap_grid.x;
        params.mouse_pos_world.y = roundf(params.mouse_pos_world.y / params.snap_grid.y) * params.snap_grid.y;

        // Clip drawing to canvas area
        ImGui::PushClipRect(canvas_pos, canvas_pos + canvas_size, true);

        return true;
    }

    void EndCanvas()
    {
        ImGui::PopClipRect();
        ImGui::EndChild();
    }

    inline float ComputeDynamicStep(float base_step, float zoom, float min_pixel_spacing = 30.0f)
    {
        float scaled = base_step * zoom;
        int   exp    = 0;

        while (scaled < min_pixel_spacing)
        {
            scaled *= 2.0f;
            exp++;
        }

        return base_step * std::pow(2.0f, exp);
    }

    void DrawRuler(const CanvasParams& params, ImVec2 grid_step, ImU32 color)
    {
        auto* draw = ImGui::GetWindowDrawList();

        ImVec2 dynamic_step{ComputeDynamicStep(grid_step.x, params.zoom), ComputeDynamicStep(grid_step.y, params.zoom)};

        const ImVec2 canvas_min = params.canvas_pos;
        const ImVec2 canvas_max = canvas_min + params.canvas_size;

        ImFont* font        = ImGui::GetFont();
        float   font_height = font->FontSize;

        // Horizontal ruler
        float start_x = floorf(params.ScreenToWorld(canvas_min).x / dynamic_step.x) * dynamic_step.x;
        float end_x   = params.ScreenToWorld(canvas_max).x;

        for (float x = start_x; x < end_x; x += dynamic_step.x)
        {
            ImVec2 p = params.WorldToScreen(ImVec2(x, 0));
            draw->AddLine(ImVec2(p.x, canvas_min.y), ImVec2(p.x, canvas_min.y + 6), color);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", x);
            draw->AddText(ImVec2(p.x + 2, canvas_min.y + 6), color, buf);
        }

        // Vertical ruler
        float start_y = floorf(params.ScreenToWorld(canvas_min).y / dynamic_step.y) * dynamic_step.y;
        float end_y   = params.ScreenToWorld(canvas_max).y;

        for (float y = start_y; y < end_y; y += dynamic_step.y)
        {
            ImVec2 p = params.WorldToScreen(ImVec2(0, y));
            draw->AddLine(ImVec2(canvas_min.x, p.y), ImVec2(canvas_min.x + 6, p.y), color);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", y);
            draw->AddText(ImVec2(canvas_min.x + 8, p.y - font_height * 0.5f), color, buf);
        }
    }

    void DrawGrid(const CanvasParams& params, ImVec2 grid_step, ImU32 color)
    {
        auto* draw = ImGui::GetWindowDrawList();

        ImVec2 dynamic_step{ComputeDynamicStep(grid_step.x, params.zoom), ComputeDynamicStep(grid_step.y, params.zoom)};

        const ImVec2 canvas_min = params.canvas_pos;
        const ImVec2 canvas_max = canvas_min + params.canvas_size;

        // Convert screen to world bounds
        const ImVec2 world_min = params.ScreenToWorld(canvas_min);
        const ImVec2 world_max = params.ScreenToWorld(canvas_max);

        // Snap start to nearest lower grid cell
        const float start_x = floorf(world_min.x / dynamic_step.x) * dynamic_step.x;
        const float start_y = floorf(world_min.y / dynamic_step.y) * dynamic_step.y;

        for (float x = start_x; x < world_max.x; x += dynamic_step.x)
        {
            const float sx = params.WorldToScreen(ImVec2(x, 0)).x;
            draw->AddLine(ImVec2(sx, canvas_min.y), ImVec2(sx, canvas_max.y), color);
        }

        for (float y = start_y; y < world_max.y; y += dynamic_step.y)
        {
            const float sy = params.WorldToScreen(ImVec2(0, y)).y;
            draw->AddLine(ImVec2(canvas_min.x, sy), ImVec2(canvas_max.x, sy), color);
        }
    }

    void DrawOrigin(const CanvasParams& params, float axis_length)
    {
        auto*  draw          = ImGui::GetWindowDrawList();
        ImVec2 origin_screen = params.WorldToScreen(ImVec2(0, 0));

        // Calculate line endpoints in screen space
        ImVec2 x_start, x_end, y_start, y_end;

        if (axis_length < 0)
        {
            // Extend lines beyond canvas in both directions
            // We'll just pick a large enough length in screen space (like 10k pixels)
            const float big_len = 10000.0f;

            // X axis: horizontal line
            x_start = origin_screen - ImVec2(big_len, 0);
            x_end   = origin_screen + ImVec2(big_len, 0);

            // Y axis: vertical line
            y_start = origin_screen - ImVec2(0, big_len);
            y_end   = origin_screen + ImVec2(0, big_len);
        }
        else
        {
            // Finite length in world space, convert to screen
            x_start = origin_screen;
            x_end   = params.WorldToScreen(ImVec2(axis_length, 0));

            y_start = origin_screen;
            y_end   = params.WorldToScreen(ImVec2(0, axis_length));
        }

        // Draw X axis in red
        draw->AddLine(x_start, x_end, IM_COL32(255, 0, 0, 255), 1.0f);
        // Draw Y axis in green
        draw->AddLine(y_start, y_end, IM_COL32(0, 255, 0, 255), 1.0f);

        // Optional labels at positive ends (only if finite)
        if (axis_length >= 0)
        {
            ImFont* font = ImGui::GetFont();
            if (font)
            {
                draw->AddText(x_end + ImVec2(5, -10), IM_COL32(255, 0, 0, 255), "X");
                draw->AddText(y_end + ImVec2(5, -10), IM_COL32(0, 255, 0, 255), "Y");
            }
        }
    }

    void DrawTexture(const CanvasParams& params, ImTextureID tid, ImVec2 p1, ImVec2 p2, ImVec2 uv1, ImVec2 uv2, ImU32 color)
    {
        auto* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddImage(tid, params.WorldToScreen(p1), params.WorldToScreen(p2), uv1, uv2, color);
    }

    bool OpenFileInput(const char* label, std::string& path, const char* filter)
    {
        bool ret{};
        if (BeginCombo(label, path.c_str()))
        {
            auto r = fin::open_file_dialog("Open", path, fin::create_file_filter(filter));
            if (!r.empty())
            {
                ret  = true;
                path = r[0];
            }
            ImGui::CloseCurrentPopup();
            ImGui::EndCombo();
        }
        return ret;
    }

    bool SaveFileInput(const char* label, std::string& path, const char* filter)
    {
        bool ret{};
        if (BeginCombo(label, path.c_str()))
        {
            auto r = fin::save_file_dialog("Open", path, fin::create_file_filter(filter));
            if (!r.empty())
            {
                ret  = true;
                path = r;
            }
            ImGui::CloseCurrentPopup();
            ImGui::EndCombo();
        }
        return ret;
    }




    class AtlasEditor : public Editor
    {
    public:
        bool load(std::string_view path) override
        {
            _data.atlas = fin::Atlas::load_shared(path);
            return _data.atlas.get();
        }

        bool imgui_show() override
        {
            for (auto n = 0; n < _data.atlas->size(); ++n)
            {
                auto& spr = _data.atlas->get(n + 1);
                ImGui::PushID(n);
                if (ImGui::Selectable("##id", _data.sprite == &spr, 0, {0, 25}))
                    _data.sprite = &spr;

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    _data.sprite = &spr;
                    ImGui::SetDragData(&_data, "SPRITE");
                    ImGui::SetDragDropPayload("SPRITE", &n, sizeof(int32_t));
                    ImGui::EndDragDropSource();
                }

                ImGui::SameLine();
                ImGui::SpriteImage(&spr, {24, 24});
                ImGui::SameLine();
                ImGui::Text(spr._name.c_str());
                ImGui::PopID();
            }
            return true;
        }

        fin::Atlas::Pack _data;
    };


    std::shared_ptr<Editor> Editor::load_from_file(std::string_view path)
    {
        auto n = path.rfind('.');
        if (n == std::string_view::npos)
            return nullptr;
        std::shared_ptr<Editor> out;
        auto ext = path.substr(n + 1);

        if (ext == "atlas")
            out = std::make_shared<AtlasEditor>();

        if (out && !out->load(path))
            return nullptr;

        return out;
    }


    bool CanvasParams::DragPoint(ImVec2& pt, void* user_data, float radius_screen)
    {
        if (!dragging)
        {
            ImVec2      mouse_pos = ImGui::GetIO().MousePos;
            ImVec2      pt_screen = WorldToScreen(pt);
            const float dx        = mouse_pos.x - pt_screen.x;
            const float dy        = mouse_pos.y - pt_screen.y;
            const float dist_sq   = dx * dx + dy * dy;
            const float radius_sq = radius_screen * radius_screen;

            if (dist_sq < radius_sq && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                dragging           = true;
                dragging_user_data = user_data;

                // Capture offset in world space between mouse and point
                ImVec2 mouse_world = ScreenToWorld(mouse_pos);
                drag_offset_world  = pt - mouse_world;

                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }

        return EndDrag(pt, user_data);
    }

    bool CanvasParams::BeginDrag(ImVec2& pt, void* user_data)
    {
        if (!dragging)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                dragging           = true;
                dragging_user_data = user_data;

                ImVec2 mouse_world = ScreenToWorld(ImGui::GetIO().MousePos);
                drag_offset_world  = pt - mouse_world;

                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                return true;
            }
        }

        return false;
    }

    bool CanvasParams::EndDrag(ImVec2& pt, void* user_data)
    {
        if (dragging && dragging_user_data == user_data)
        {
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImVec2 mouse_world = ScreenToWorld(mouse_pos);
                pt                 = Snap(mouse_world + drag_offset_world); // Apply offset
                return true;
            }
            else
            {
                dragging           = false;
                dragging_user_data = nullptr;
            }
        }
        return false;
    }

} // namespace ImGui
