#pragma once

#include "core/proto_scene_object.hpp"
#include "file_editor.hpp"
#include "json_editor.hpp"

namespace fin
{

class ProtoFileEdit : public FileEdit
{
public:
    bool on_init(std::string_view path) override;
    bool on_deinit() override;
    bool on_edit() override;
    bool on_setup() override;

    ProtoSceneObject _object;
    msg::Var _data;
    std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> _atlases;
};

inline bool ProtoFileEdit::on_init(std::string_view path)
{
    if (auto* txt = LoadFileText(path.data()))
    {
        msg::Var obj;
        obj.from_string(txt);
        _data = obj.get_item("items");
        UnloadFileText(txt);
    }

    return _data.is_array();
}

inline bool ProtoFileEdit::on_deinit()
{
    return false;
}

inline bool ProtoFileEdit::on_edit()
{
    if (!_data.is_array())
        return false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 16});

    if (ImGui::Selectable(ICON_FA_FOLDER " .."))
    {
        ImGui::PopStyleVar();
        return false;
    }

    ImGuiListClipper clipper;
    clipper.Begin(_data.size());
    while (clipper.Step())
    {
        for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
        {
            // Display a data item
            ImGui::PushID(row_n);

            auto obj = _data.get_item(row_n).get_item(ObjId);
            auto scn = _data.get_item(row_n).get_item(SceneObjId);
            if (scn.is_object())
            {
                auto Sprite = scn.get_item("spr");
                if (Sprite.is_array())
                {
                    auto it = _atlases.find(Sprite.get_item(0).str());
                    if (it == _atlases.end())
                    {
                        _atlases[Sprite.get_item(0).c_str()] =
                            Atlas::load_shared(Sprite.get_item(0).str());
                    }
                    else if (auto n = it->second->find_sprite(Sprite.get_item(1).str()))
                    {
                        auto &sp = it->second->get(n);
                        ImGui::Image(
                            (ImTextureID)sp._texture,
                            {24, 24},
                            {sp._source.x / sp._texture->width, sp._source.y / sp._texture->height},
                            {sp._source.x2() / sp._texture->width,
                             sp._source.y2() / sp._texture->height});
                        ImGui::SameLine();
                    }
                }
            }

            ImGui::Selectable(obj.c_str());

            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                _object.load_prototype(_data.get_item(row_n));
                ImGui::SetDragData(&_object);
                ImGui::SetDragDropPayload("PROTO", &row_n, sizeof(int32_t));
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
        }
    }

    ImGui::PopStyleVar();
    return true;
}

inline bool ProtoFileEdit::on_setup()
{
    return false;
}

} // namespace fin
