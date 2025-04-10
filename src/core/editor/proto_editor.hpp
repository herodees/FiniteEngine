#pragma once

#include "core/atlas_scene_object.hpp"
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

    AtlasSceneObject _object;
    msg::Var _data;
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
            auto obj = _data.get_item(row_n).get_item(SceneObjId);
            auto label = obj.get_item("id");

            // Display a data item
            ImGui::PushID(row_n);
            ImGui::Selectable(label.c_str());
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
