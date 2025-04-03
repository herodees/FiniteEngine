#include "json_editor.hpp"

namespace fin
{

bool JsonEdit::show(msg::Var &data)
{
    auto sch_items = _schema["properties"];
    if (!data.is_object())
    {
        data.make_object(1);
    }
    return show_object(sch_items, data);
}

void JsonEdit::schema(const char *sch)
{
    _schema.from_string(sch);
}

void JsonEdit::schema(msg::Var &sch)
{
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
        if (ImGui::Button("X"))
            del = val.n;

        ImGui::PopID();
        ++val.n;
    }

    if (del >= 0)
    {
        data.erase(del);
        modified = true;
    }

    if (ImGui::Button("Add"))
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

    for (auto el : sch.members())
    {
        val.k = el.first.str();
        if (show_schema(el.second, val, val.k))
            modified = true;
    }
    return false;
}

bool JsonEdit::show_schema(msg::Var &sch, Val &val, std::string_view key)
{
    bool modified = false;
    auto type_el = sch.get_item("type");
    auto type = type_el.str();

    if(type.empty())
    {
        type_el = sch.get_item("enum");
        if (type_el.is_array())
        {
            auto v = val.get_item();
            if (ImGui::BeginCombo(key.data(), v.c_str()))
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
    }
    else if (type == "string")
    {
        _buffer = val.get_item().str();

        if (ImGui::InputText(key.data(), &_buffer))
        {
            val.set_item(_buffer);
            modified = true;
        }
    }
    else if (type == "number")
    {
        float value = val.get_item().get(0.0f);
        if (ImGui::InputFloat(key.data(), &value))
        {
            val.set_item(value);
            modified = true;
        }
    }
    else if (type == "integer")
    {
        int value = val.get_item().get(0);
        if (ImGui::InputInt(key.data(), &value))
        {
            val.set_item(value);
            modified = true;
        }
    }
    else if (type == "color")
    {
        int value = std::hex_to_color(val.get_item().str(), 0);
        auto color = ImGui::ColorConvertU32ToFloat4(value);
        if (ImGui::ColorEdit4(key.data(),
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
        bool value = val.get_item().get(false);
        if (ImGui::Checkbox(key.data(), &value))
        {
            val.set_item(value);
            modified = true;
        }
    }
    else if (type == "array")
    {
        if (!val.get_item().is_array())
        {
            fin::msg::Var arr;
            arr.make_array(1);
            val.set_item(arr);
        }

        if (ImGui::CollapsingHeader(key.data()))
        {
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

        if (ImGui::CollapsingHeader(key.data()))
        {
            ImGui::Indent();
            auto item = val.get_item();
            auto sch_items = sch["properties"];
            modified |= show_object(sch_items, item);
            ImGui::Unindent();
        }
    }

    return false;
}

} // namespace fin
