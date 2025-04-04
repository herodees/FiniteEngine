#pragma once

#include "file_editor.hpp"
#include "core/atlas.hpp"
#include "core/atlas_scene_object.hpp"

namespace fin
{
    class AtlasFileEdit : public FileEdit
    {
    public:
        bool on_init(std::string_view path) override;
        bool on_deinit() override;
        bool on_edit() override;
        bool on_setup() override;

        std::shared_ptr<Atlas> _atlas;
        AtlasSceneObject _object;
    };

    inline bool AtlasFileEdit::on_init(std::string_view path)
    {
        _atlas = Atlas::load_shared(path);

        return _atlas.get();
    }

    inline bool AtlasFileEdit::on_deinit()
    {
        _atlas.reset();
        return false;
    }

    inline bool AtlasFileEdit::on_edit()
    {
        if (!_atlas)
            return false;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 16});

        if (ImGui::Selectable(ICON_FA_FOLDER " .."))
        {
            ImGui::PopStyleVar();
            return false;
        }

        for (uint32_t n = 0; n < _atlas->size(); ++n)
        {
            auto& spr = _atlas->get(n + 1);

            ImGui::PushID(n); // Ensure unique ID per item

            ImVec2 sze(spr._source.width, spr._source.height);
            float max_dim = (sze.x > sze.y) ? sze.x : sze.y; // Get max dimension
            float scale = 24.0f / max_dim; // Scale based on the larger dimension

            sze.x *= scale;
            sze.y *= scale;

            ImVec2 uv1(spr._source.x / spr._texture->width, spr._source.y / spr._texture->height);
            ImVec2 uv2(spr._source.x2() / spr._texture->width, spr._source.y2() / spr._texture->height);

            auto xoff = ImGui::GetCursorPosX();
            ImGui::Image((ImTextureID)spr._texture, sze, uv1, uv2); // Render the icon (Adjust size)
            ImGui::SameLine(); // Keep icon and text on the same line
            ImGui::SetCursorPosX(xoff + 24);
            ImGui::Selectable(spr._name.c_str());

            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                _object.set_atlas(_atlas);
                _object.set_sprite(&_atlas->get(n + 1));
                ImGui::SetDragData(&_object);
                ImGui::SetDragDropPayload("SPRITE", &n, sizeof(uint32_t));
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
        }

        ImGui::Separator();
        for (uint32_t n = 0; n < _atlas->size_c(); ++n)
        {
            auto& spr = _atlas->get_c(n + 1);

            ImGui::PushID(n); // Ensure unique ID per item

            if (ImGui::Selectable(spr._name.c_str()))
            {
            }
            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragData(&_object);
                ImGui::SetDragDropPayload("COMPOSITE", &n, sizeof(uint32_t));
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
        }

        ImGui::PopStyleVar();

        return true;
    }

    inline bool AtlasFileEdit::on_setup()
    {
        return false;
    }

    }
