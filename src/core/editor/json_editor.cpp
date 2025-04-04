#include "json_editor.hpp"

namespace fin
{

bool JsonEdit::show(msg::Var &data)
{
    _root = &data;
    auto sch_items = _schema["properties"];
    if (!data.is_object())
    {
        data.make_object(1);
    }
    return show_object(sch_items, data);
}

void JsonEdit::schema(const char *sch)
{
    _atlases.clear();
    _schema.from_string(sch);
}

void JsonEdit::schema(msg::Var &sch)
{
    _atlases.clear();
    _schema = sch;
}

bool JsonEdit::show_array(msg::Var &sch, msg::Var &data)
{
    if (!sch.is_object())
        return false;

    bool modified = false;
    Val val{};
    val.p = data;
    int del = -1;

    for (auto el : data.elements())
    {
        ImGui::PushID(val.n);
        if (show_schema(sch, val, ImGui::FormatStr("%d", val.n)))
            modified = true;
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH))
            del = val.n;

        ImGui::PopID();
        ++val.n;
    }

    if (del >= 0)
    {
        data.erase(del);
        modified = true;
    }

    if (ImGui::Button(ICON_FA_FILE " Add"))
    {
        data.push_back(nullptr);
        modified = true;
    }

    return modified;
}

bool JsonEdit::show_object(msg::Var &sch, msg::Var &data)
{
    if (!sch.is_object())
        return false;

    bool modified = false;
    Val val;
    val.p = data;
    val.n = 0;

    for (auto el : sch.members())
    {
        ImGui::PushID(val.n);
        val.k = el.first.str();
        if (show_schema(el.second, val, val.k))
            modified = true;
        ImGui::PopID();
        ++val.n;
    }
    return false;
}

bool JsonEdit::show_schema(msg::Var &sch, Val &val, std::string_view key)
{
    bool modified = false;
    auto type_el = sch.get_item("type");
    auto descr_el = sch.get_item("description");
    auto title_el = sch.get_item("title");
    auto help_el = sch.get_item("help");
    auto def_el = sch.get_item("default");
    auto type = type_el.str();
    auto descr = descr_el.str();
    auto title = title_el.get(key);
    auto help = help_el.str();

    if(type.empty())
    {
        type_el = sch.get_item("enum");
        if (type_el.is_array())
        {
            auto v = val.get_item();
            if (ImGui::BeginCombo(title.data(), v.c_str()))
            {
                for (auto el : type_el.elements())
                {
                    if (ImGui::Selectable(el.c_str()))
                    {
                        val.set_item(el);
                        modified = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            type_el = sch.get_item("$ref");
            if (type_el.is_string())
            {
                auto pth = type_el.str();
                if (pth.size() > 2 && pth[0] == '#' && pth[1] == '/')
                {
                    auto ref = _schema.find(pth.substr(2));
                    return show_schema(ref, val, key);
                }
            }
        }

    }
    else if (type == "sprite")
    {
        auto spr = val.get_item();
        msg::Var sprite_path;

        if (spr.size() == 2)
        {
            _buffer = spr.get_item(0).str();
            sprite_path = spr.get_item(1).str();
        }

        if (ImGui::OpenFileInput(title.data(), _buffer, "Atlas file|*.atlas|JSON atlas|*.json|All items|*"))
        {
            if (!spr.is_array())
            {
                spr.make_array(2);
                val.set_item(spr);
            }

            spr.set_item(0, _buffer);
            spr.set_item(1, "");

            modified = true;
        }

        if (spr.size() == 2)
        {
            auto it = _atlases.find(_buffer);
            if (it == _atlases.end())
            {
                _atlases[_buffer] = Atlas::load_shared(_buffer);
            }
            else if (auto n = it->second->find_sprite(sprite_path.str()))
            {
                auto &sp = it->second->get(n);
                ImGui::Image(
                    (ImTextureID)sp._texture,
                    {32, 32},
                    {sp._source.x / sp._texture->width, sp._source.y / sp._texture->height},
                    {sp._source.x2() / sp._texture->width, sp._source.y2() / sp._texture->height});
                ImGui::SameLine();
            }

            if (ImGui::BeginCombo("##sprid", sprite_path.c_str()))
            {
                for (auto n = 0; n < it->second->size(); ++n)
                {
                    auto &sp = it->second->get(n + 1);
                    if (ImGui::Selectable(sp._name.c_str(), sp._name == sprite_path.c_str()))
                    {
                        spr.set_item(1, sp._name);
                        modified = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
    else if (type == "string")
    {
        _buffer = val.get_item().get(def_el.str());

        if (ImGui::InputText(title.data(), &_buffer))
        {
            val.set_item(_buffer);
            modified = true;
        }
    }
    else if (type == "number")
    {
        float value = val.get_item().get(def_el.get(0.0f));
        if (ImGui::InputFloat(title.data(), &value))
        {
            if (sch.contains("minimum"))
                value = std::max(value, sch.get_item("minimum").get(0.0f));
            if (sch.contains("maximum"))
                value = std::min(value, sch.get_item("maximum").get(0.0f));
            val.set_item(value);
            modified = true;
        }
    }
    else if (type == "integer")
    {
        int value = val.get_item().get(def_el.get(0));

        if (ImGui::InputInt(title.data(), &value))
        {
            val.set_item(std::clamp(value,
                                    sch.get_item("minimum").get(INT_MIN),
                                    sch.get_item("maximum").get(INT_MAX)));
            modified = true;
        }
    }
    else if (type == "color")
    {
        int value = std::hex_to_color(val.get_item().get(def_el.get("")), 0);
        auto color = ImGui::ColorConvertU32ToFloat4(value);
        if (ImGui::ColorEdit4(title.data(),
                              &color.x,
                              ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel))
        {
            value = ImGui::ColorConvertFloat4ToU32(color);
            val.set_item(std::color_to_hex(value));
            modified = true;
        }
    }
    else if (type == "boolean")
    {
        bool value = val.get_item().get(def_el.get(false));
        if (ImGui::Checkbox(title.data(), &value))
        {
            val.set_item(value);
            modified = true;
        }
    }
    else
    {
        if (type == "array")
        {
            if (!val.get_item().is_array())
            {
                fin::msg::Var arr;
                arr.make_array(1);
                val.set_item(arr);
            }

            if (ImGui::CollapsingHeader(title.data()))
            {
                if (!descr.empty())
                    ImGui::Text(descr.data());

                ImGui::Indent();
                auto sch_items = sch["items"];
                auto data_key = val.get_item();
                modified |= show_array(sch_items, data_key);
                ImGui::Unindent();
            }
        }
        else if (type == "object")
        {
            if (!val.get_item().is_object())
            {
                fin::msg::Var obj;
                obj.make_object(1);
                val.set_item(obj);
            }

            if (ImGui::CollapsingHeader(title.data()))
            {
                if (!descr.empty())
                    ImGui::Text(descr.data());

                ImGui::Indent();
                auto item = val.get_item();
                auto sch_items = sch["properties"];
                modified |= show_object(sch_items, item);
                ImGui::Unindent();
            }
        }

        return modified;
    }

    if (!descr.empty())
        ImGui::Text(descr.data());

    if (!help.empty())
    {
        ImGui::SameLine();
        ImGui::HelpMarker(descr.data());
    }
    return modified;
}

} // namespace fin
