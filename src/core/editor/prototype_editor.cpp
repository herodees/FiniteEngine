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
        msg::Var data;
        auto path = sch.get_item("path");
        if (!path.is_string())
        {
            TraceLog(LOG_ERROR,
                     "Prototype \"%s\": Missing 'path' attribute!",
                     _prototypes.back().first.c_str());
        }
        else
        {
            if (auto* txt = LoadFileText(path.c_str()))
            {
                msg::Var obj;
                obj.from_string(txt);
                data = obj.get_item("items");
                UnloadFileText(txt);
            }
            else
            {
                data.make_object(1);
                TraceLog(LOG_ERROR,
                         "Prototype \"%s\": File '%s' load failed!",
                         _prototypes.back().first.c_str(),
                         path.c_str());
            }
        }
        _data.emplace_back(data);
    }
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
    if (_active_tab >= _data.size())
        return false;
    if (_active_item >= _data[_active_tab].size())
        return false;

    auto object = _data[_active_tab].get_item(_active_item);
    auto scene_object = object.get_item(SceneObjId);
    auto sprite_info = scene_object.get_item("spr");
    std::shared_ptr<Atlas> atlas_ptr;
    Atlas::sprite *atlas_spr{};
    if (sprite_info.is_array())
    {
        auto atl_path = sprite_info.get_item(0);
        auto spr_path = sprite_info.get_item(1);
        atlas_ptr = Atlas::load_shared(atl_path.str());
        if (atlas_ptr)
            if (auto n = atlas_ptr->find_sprite(spr_path.str()))
                atlas_spr = &atlas_ptr->get(n);

    }

    static int s_drag_point = 0;
    static ImVec2 s_prev_mpos;
    static std::vector<ImVec2> s_points;
    if (!ImGui::IsMouseDown(0))
    {
        s_drag_point = 0;
    }
    Scene::Params params;

    ImVec2 visible_size = ImGui::GetContentRegionAvail();
    params.pos = ImGui::GetWindowPos();
    auto cur = ImGui::GetCursorPos();
    params.dc = ImGui::GetWindowDrawList();
    auto mpos = ImGui::GetMousePos();

    params.scroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
    params.pos.x -= ImGui::GetScrollX();
    params.pos.y -= ImGui::GetScrollY();

    ImGui::InvisibleButton("Canvas", ImVec2(2000, 2000));
    params.pos.x += 1000;
    params.pos.y += 1000;

    params.mouse = {mpos.x - params.pos.x,
                    mpos.y - params.pos.y};

    if (_scroll_to_center)
    {
        _scroll_to_center = false;
        ImGui::SetScrollHereX();
        ImGui::SetScrollHereY();
    }

    if (atlas_spr)
    {
        Region<float> reg(params.pos.x - atlas_spr->_origina.x,
                          params.pos.y - atlas_spr->_origina.y,
                          params.pos.x + atlas_spr->_source.width - atlas_spr->_origina.x,
                          params.pos.y + atlas_spr->_source.height - atlas_spr->_origina.y);

        ImVec2 txs(atlas_spr->_texture->width, atlas_spr->_texture->height);
        params.dc->AddImage((ImTextureID)atlas_spr->_texture,
                            {reg.x1, reg.y1},
                            {reg.x2, reg.y2},
                            {atlas_spr->_source.x / txs.x, atlas_spr->_source.y / txs.y},
                            {atlas_spr->_source.x2() / txs.x, atlas_spr->_source.y2() / txs.y});

        params.dc->AddRect({reg.x1, reg.y1}, {reg.x2, reg.y2}, 0x7fffff00);

        if (_edit_origin)
        {
            auto isoa = scene_object.get_item("isoa");
            if (!isoa.is_array())
            {
                isoa.make_array(2);
                scene_object.set_item("isoa", isoa);
            }
            auto isob = scene_object.get_item("isob");
            if (!isob.is_array())
            {
                isob.make_array(2);
                scene_object.set_item("isob", isob);
            }

            ImVec2 a{params.pos.x + isoa.get_item(0).get(0.0f),
                     params.pos.y + isoa.get_item(1).get(0.0f)};
            ImVec2 b{params.pos.x + isob.get_item(0).get(0.0f),
                     params.pos.y + isob.get_item(1).get(0.0f)};

            bool hovera = params.mouse_distance2(a - params.pos) < 5 * 5;
            bool hoverb = params.mouse_distance2(b - params.pos) < 5 * 5;

            if (ImGui::IsItemClicked(0))
            {
                if (hovera || hoverb)
                {
                    s_prev_mpos = params.mouse;
                    s_drag_point = hovera ? 1 : 2;
                }
            }

            if (s_drag_point)
            {
                auto diff = s_prev_mpos - params.mouse;
                s_prev_mpos = params.mouse;
                auto iso = s_drag_point == 1 ? isoa : isob;
                iso.set_item(0, iso.get_item(0).get(0.0f) - diff.x);
                iso.set_item(1, iso.get_item(1).get(0.0f) - diff.y);
            }

            params.dc->AddLine(a, b, 0xff909090, 2);
            hovera ? params.dc->AddCircleFilled(a, 5, 0xff00ff00)
                   : params.dc->AddCircle(a, 5, 0xff00ff00);
            hoverb ? params.dc->AddCircleFilled(b, 5, 0xff0000ff)
                   : params.dc->AddCircle(b, 5, 0xff0000ff);
        }

        if (_edit_collision)
        {
            auto coll = scene_object.get_item("coll");
            if (!coll.is_array())
            {
                coll.make_array(2);
                scene_object.set_item("coll", coll);
            }

            s_points.clear();
            for (auto n = 0; n < coll.size(); n+=2)
            {
                ImVec2 a{params.pos.x + coll.get_item(n).get(0.0f),
                         params.pos.y + coll.get_item(n+1).get(0.0f)};
                bool hovera = params.mouse_distance2(a - params.pos) < 5 * 5;
                if (ImGui::IsItemClicked(0))
                {
                    if (hovera)
                    {
                        s_prev_mpos = params.mouse;
                        s_drag_point = n+1;
                    }
                }

                if (s_drag_point)
                {
                    auto diff = s_prev_mpos - params.mouse;
                    s_prev_mpos = params.mouse;
                    coll.set_item(s_drag_point - 1,
                                  coll.get_item(s_drag_point - 1).get(0.0f) - diff.x);
                    coll.set_item(s_drag_point, coll.get_item(s_drag_point).get(0.0f) - diff.y);
                }

                s_points.push_back(a);
                hovera ? params.dc->AddCircleFilled(a, 5, 0xff00ff00)
                       : params.dc->AddCircle(a, 5, 0xff00ff00);
            }

            if (s_points.size() >= 3)
            {
                params.dc->AddPolyline(s_points.data(), (int)s_points.size(), 0xff909090, ImDrawFlags_Closed, 2);
            }
        }
    }

    params.dc->AddLine(params.pos - ImVec2(0, 1000), params.pos + ImVec2(0, 1000), 0x7f00ff00);
    params.dc->AddLine(params.pos - ImVec2(1000, 0), params.pos + ImVec2(1000, 0), 0x7f0000ff);

    return false;
}

bool PrototypeEditor::show_menu()
{
    if (ImGui::RadioButton("Select", (!_edit_collision && !_edit_origin)))
    {
        _edit_origin = false;
        _edit_collision = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Origin", _edit_origin))
    {
        _edit_origin = true;
        _edit_collision = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Collision", _edit_collision))
    {
        _edit_origin = false;
        _edit_collision = true;
    }
    return false;
}

void PrototypeEditor::scroll_to_center()
{
    _scroll_to_center = true;
}

bool PrototypeEditor::show_proto_props()
{
    _active_item = _prototypes[_active_tab].second.get_item("@sel").get(0);

    if (ImGui::Button("Add"))
    {
        msg::Var obj;
        obj.make_object(1);
        _data[_active_tab].push_back(obj);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        auto path = _prototypes[_active_tab].second.get_item("path");
        if (!path.is_string())
        {
            TraceLog(LOG_ERROR,
                     "Prototype \"%s\": Missing 'path' attribute!",
                     _prototypes[_active_tab].first.c_str());
        }
        else
        {
            msg::Var obj;
            obj.set_item("items", _data[_active_tab]);
            std::string str;
            obj.to_string(str, true);
            if (!SaveFileText(path.c_str(), str.data()))
            {
                TraceLog(LOG_ERROR,
                         "Prototype \"%s\": File '%s' save failed!",
                         _prototypes[_active_tab].first.c_str(),
                         path.c_str());
            }
        }
    }
    ImGui::SameLine();
    ImGui::Dummy({ImGui::GetContentRegionAvail().x - 32, 1});
    ImGui::SameLine();
    if (ImGui::Button("Del"))
    {
        if (_active_item < _data[_active_tab].size())
            _data[_active_tab].erase(_active_item);
    }

    if (ImGui::BeginChildFrame(1, { -1, 150 }))
    {
        int32_t n = 0;
        for (auto el : _data[_active_tab].elements())
        {
            ImGui::PushID(n);
            if (ImGui::Selectable(_prototypes[_active_tab].first.c_str(), _active_item == n))
            {
                _active_item = n;
            }
            ImGui::PopID();
            ++n;
        }
    }
    ImGui::EndChildFrame();

    if (_active_item < _data[_active_tab].size())
    {
        _prototypes[_active_tab].second.set_item("@sel", _active_item);
        auto obj = _data[_active_tab].get_item(_active_item);
        return _json_edit.show(obj);
    }

    return false;
}

} // namespace fin
