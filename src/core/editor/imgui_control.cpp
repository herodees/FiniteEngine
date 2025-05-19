#include "imgui_control.hpp"
#include "extras/IconsFontAwesome6.h"
#include "misc/cpp/imgui_stdlib.cpp"
#include "utils/dialog_utils.hpp"

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

    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size, bool scene_edit)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));

        ImGui::Text(label);
        if (ImGui::BeginChildFrame(-2, size))
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


} // namespace ImGui
