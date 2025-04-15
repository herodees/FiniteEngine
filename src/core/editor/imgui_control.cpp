#include "imgui_control.hpp"

namespace ImGui
{
    struct SharedData
    {
        std::string buff;
    } s_shared;


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

    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size)
    {
        bool modified = false;
        ImGui::PushID(ImGui::GetID(label));



        ImGui::Text(label);
        if (ImGui::BeginChildFrame(-1, size))
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

} // namespace ImGui
