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
    } s_shared;


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

    bool PointVector(const char* label, std::vector<fin::Vec2f>* points, ImVec2 size, int* active)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));

        ImGui::LabelText(label, "");
        if (ImGui::BeginChildFrame(ImGui::GetID("pts"), size))
        {
            for (auto n = 0; n < points->size(); ++n)
            {
                ImGui::PushID(n);
                if (active && *active == n)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
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
                if (active && *active == n)
                {
                    ImGui::PopStyleColor();
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

    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size, int* active)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));

        ImGui::Text(label);
        if (ImGui::BeginChildFrame(ImGui::GetID("pts"), size))
        {
            for (auto n = 0; n < points->size(); n += 2)
            {
                ImGui::PushID(n);
                if (active && *active == n / 2)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
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
                if (active && *active == n / 2)
                {
                    ImGui::PopStyleColor();
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

    void SpriteImage(fin::Sprite2D* spr, ImVec2 size)
    {
        if (spr && spr->GetTexture())
        {
            auto  uv  = spr->GetUVRegion();
            auto  sze = spr->GetSize();
            float sz  = std::max(sze.x, sze.y);

            float msx = size.x / sz;
            float msy = size.y / sz;

            ImVec2 actual_size = {sze.x * msx, sze.y * msy};
            ImVec2 offset = {(size.x - actual_size.x) * 0.5f, (size.y - actual_size.y) * 0.5f};

            ImVec2 cursor = ImGui::GetCursorPos();
            ImGui::SetCursorPos({cursor.x + offset.x, cursor.y + offset.y});

            ImGui::Image((ImTextureID)spr->GetTexture()->get_texture(),
                         actual_size,
                         ImVec2{uv.x1, uv.y1},
                         ImVec2{uv.x2, uv.y2});

            ImGui::SetCursorPos(cursor); // Reset before dummy
        }
        ImGui::Dummy(size);
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
            auto r = fin::OpenFileDialog("Open", path, fin::CreateFileFilter(filter));
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
            auto r = fin::SaveFileDialog("Open", path, fin::CreateFileFilter(filter));
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

    bool SpriteInput(const char* label, fin::Sprite2D::Ptr* sprite)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));
        auto sze = ImGui::GetFrameHeight();
        if (sprite->get() && sprite->get()->GetTexture())
        {
            auto reg = sprite->get()->GetUVRegion();
            ImGui::Image((ImTextureID)sprite->get()->GetTexture()->get_texture(),
                         {sze, sze},
                         {reg.x1, reg.y1},
                         {reg.x2, reg.y2});


            if (ImGui::BeginItemTooltip())
            {
                auto size = sprite->get()->GetSize();
                ImGui::Image((ImTextureID)sprite->get()->GetTexture()->get_texture(),
                             {size.x, size.y},
                             {reg.x1, reg.y1},
                             {reg.x2, reg.y2});
                ImGui::Text("Path: %s", sprite->get()->GetPath().c_str());
                ImGui::Text("Size: %.0fx%.0f", size.x, size.y);
                ImGui::EndTooltip();
            }

            ImGui::SameLine();
        }
        else
        {
            ImGui::InvisibleButton("##img", {sze, sze});
            ImGui::SameLine();
        }

        if (ImGui::BeginCombo(label, sprite->get() ? sprite->get()->GetPath().c_str() : "None"))
        {
            if (ImGui::IsWindowAppearing())
            {
                s_active         = ".";
                s_root._expanded = false;
            }
            if (MenuItem("None", nullptr, !sprite->get()))
            {
                *sprite = nullptr;
                modified = true;
            }
            std::string path;
            if (FileMenu(path, ".sprite"))
            {
                *sprite = fin::Sprite2D::LoadShared(path);
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE2D"))
            {
                IM_ASSERT(payload->DataSize == sizeof(fin::Sprite2D*));
                if (auto* payload_n = *(fin::Sprite2D**)payload->Data)
                {
                    *sprite = payload_n->shared_from_this();
                }
            }
            ImGui::EndDragDropTarget();
        }


        ImGui::PopID();
        return modified;
    }


    std::shared_ptr<Editor> Editor::load_from_file(std::string_view path)
    {
        auto n = path.rfind('.');
        if (n == std::string_view::npos)
            return nullptr;
        std::shared_ptr<Editor> out;
        auto ext = path.substr(n + 1);

        if (out && !out->Load(path))
            return nullptr;

        return out;
    }


    std::vector<Dialog::Info> Dialog::s_activeDialogs;

    void Dialog::Close(const Ptr& dialog)
    {
        if (!dialog)
            return; 
        s_activeDialogs.erase(std::remove_if(s_activeDialogs.begin(),
                                             s_activeDialogs.end(),
                                             [&dialog](const Info& nfo)
                                             {
                                                 if (nfo.dialog == dialog)
                                                 {
                                                     return true;
                                                 }
                                                 return false;
                                             }),
                             
                              s_activeDialogs.end());
    }

    void Dialog::Show(const char* label, ImVec2 size, ImGuiWindowFlags flags, Ptr dialog)
    {
        if (!dialog)
            return;

        if (std::find_if(s_activeDialogs.begin(),
                         s_activeDialogs.end(),
                         [&dialog](const Info& nfo)
                         {
                             if (nfo.dialog == dialog)
                             {
                                 return true;
                             }
                             return false;
                         }) == s_activeDialogs.end())
        {
            auto& dlg = s_activeDialogs.emplace_back();
            dlg.dialog = dialog;
            dlg.size   = size;
            dlg.label  = label;
            dlg.flags  = flags;
        }
    }

    void Dialog::Update()
    {
        s_activeDialogs.erase(std::remove_if(s_activeDialogs.begin(),
                                             s_activeDialogs.end(),
                                             [](const Info& nfo)
                                             {
                                                 ImGuiViewport* viewport = ImGui::GetMainViewport();
                                                 ImVec2         center   = viewport->GetCenter();
                                                 ImVec2         win_pos  = ImVec2(center.x - nfo.size.x * 0.5f,
                                                                         center.y - nfo.size.y * 0.5f);
                                                 ImGui::SetNextWindowPos(win_pos, ImGuiCond_Appearing);
                                                 ImGui::SetNextWindowSize(nfo.size, ImGuiCond_Appearing);
                                                 bool opened = true;
                                                 if (ImGui::Begin(nfo.label.c_str(), &opened, nfo.flags))
                                                 {
                                                     if (!nfo.dialog->OnUpdate())
                                                     {
                                                         ImGui::End();
                                                         return true;
                                                     }
                                                 }
                                                 ImGui::End();
                                                 return !opened;
                                             }),
                              s_activeDialogs.end());
    }

} // namespace ImGui
