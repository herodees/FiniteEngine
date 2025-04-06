#include "prototype_editor.hpp"
#include "json_blobs.hpp"
#include "core/scene.hpp"

namespace fin
{

PrototypeEditor::PrototypeEditor()
{
    for (auto* str : json_blobs)
    {
        msg::Var sch;
        sch.from_string(str);
        _prototypes.emplace_back(sch.get_item("title").str(), sch);
    }
    _data.resize(_prototypes.size());
}

PrototypeEditor::~PrototypeEditor()
{

}

bool PrototypeEditor::show_props()
{
    _active_tab = -1;
    bool ret{};
    if (ImGui::BeginTabBar("ItemTabs"))
    {
        for (size_t n = 0; n < _prototypes.size(); ++n)
        {
            if (ImGui::BeginTabItem(_prototypes[n].first.c_str()))
            {
                _active_tab = n;
                if (_prev_active_tab != _active_tab)
                {
                    _prev_active_tab = n;
                    _json_edit.schema(_prototypes[n].second);
                }
                ret = show_proto_props();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    return ret;
}

bool PrototypeEditor::show_workspace()
{
    Scene::Params params;

    ImVec2 visible_size = ImGui::GetContentRegionAvail();
    params.pos = ImGui::GetWindowPos();
    auto cur = ImGui::GetCursorPos();
    params.dc = ImGui::GetWindowDrawList();
    auto mpos = ImGui::GetMousePos();
    params.mouse = {mpos.x - params.pos.x + ImGui::GetScrollX(),
                    mpos.y - params.pos.y + ImGui::GetScrollY()};
    params.scroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
    params.pos.x -= ImGui::GetScrollX();
    params.pos.y -= ImGui::GetScrollY();

    ImGui::InvisibleButton("Canvas", ImVec2(2000, 2000));

    params.pos.x += 1000;
    params.pos.y += 1000;
    params.dc->AddLine(params.pos - ImVec2(0, 1000), params.pos + ImVec2(0, 1000), 0x7f00ff00);
    params.dc->AddLine(params.pos - ImVec2(1000, 0), params.pos + ImVec2(1000, 0), 0x7f0000ff);

    return false;
}

bool PrototypeEditor::show_proto_props()
{
    auto sel = _prototypes[_active_tab].second.get_item("@sel").get(0);

    if (ImGui::Button("Add"))
    {
        msg::Var obj;
        obj.make_object(1);
        _data[_active_tab].push_back(obj);
    }
    ImGui::SameLine();
    ImGui::Dummy({ImGui::GetContentRegionAvail().x - 32, 1});
    ImGui::SameLine();
    if (ImGui::Button("Del"))
    {
        if (sel < _data[_active_tab].size())
            _data[_active_tab].erase(sel);
    }

    if (ImGui::BeginChildFrame(1, { -1,150 }))
    {
        int32_t n = 0;
        for (auto el : _data[_active_tab].elements())
        {
            ImGui::PushID(n);
            if (ImGui::Selectable(_prototypes[_active_tab].first.c_str(), sel == n))
            {
                sel = n;
            }
            ImGui::PopID();
            ++n;
        }
    }
    ImGui::EndChildFrame();

    if (sel < _data[_active_tab].size())
    {
        _prototypes[_active_tab].second.set_item("@sel", sel);
        auto obj = _data[_active_tab].get_item(sel);
        return _json_edit.show(obj);
    }

    return false;
}

} // namespace fin
