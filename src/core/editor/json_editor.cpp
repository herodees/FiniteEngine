#include "json_editor.hpp"

namespace fin
{

class JsonTypePoint : public JsonType
{
public:
    JsonTypePoint() : JsonType("point"){};

    bool ImguiProps(msg::Var &sch, JsonVal &val, std::string_view key) override
    {
        auto def_el = sch.get_item("default");
        auto pt = val.get_item();
        float v[2] = {pt.get_item(0).get(def_el.get_item(0).get(0.0f)),
                      pt.get_item(1).get(def_el.get_item(1).get(0.0f))};
        if (ImGui::InputFloat2(key.data(), v))
        {
            if (!pt.is_array())
            {
                pt.make_array(2);
                val.set_item(pt);
            }
            pt.set_item(0, v[0]);
            pt.set_item(1, v[1]);
            return true;
        }
        return false;
    }
};

class JsonTypeColor : public JsonType
{
public:
    JsonTypeColor() : JsonType("color"){};

    bool ImguiProps(msg::Var &sch, JsonVal &val, std::string_view key) override
    {
        auto def_el = sch.get_item("default");
        int value = std::hex_to_color(val.get_item().get(def_el.get("")), 0);
        auto color = ImGui::ColorConvertU32ToFloat4(value);
        if (ImGui::ColorEdit4(key.data(),
                              &color.x,
                              ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel))
        {
            value = ImGui::ColorConvertFloat4ToU32(color);
            val.set_item(std::color_to_hex(value));
            return true;
        }
        return false;
    }
};

class JsonTypePoints : public JsonType
{
public:
    JsonTypePoints() : JsonType("point_array"){};

    bool ImguiProps(msg::Var &sch, JsonVal &value, std::string_view key) override
    {
        bool modified = false;
        auto vals = value.get_item();
        auto height = ImGui::GetFrameHeightWithSpacing() * sch.get_item("lines").get(4);
        auto limit = (float)sch.get_item("limit").get(-1);

        ImGui::LabelText(key.data(), "");
        if (ImGui::BeginChildFrame(-1, {-1, height}))
        {
            for (uint32_t n = 0; n < vals.size(); n += 2)
            {
                float v[2] = {vals.get_item(n).get(0.f), vals.get_item(n + 1).get(0.f)};
                if (ImGui::InputFloat2(ImGui::FormatStr("%d", n / 2), v))
                {
                    vals.set_item(n, v[0]);
                    vals.set_item(n + 1, v[1]);
                    modified = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(ImGui::FormatStr(ICON_FA_TRASH "##x%d", n / 2)))
                {
                    vals.erase(n);
                    vals.erase(n);
                    modified = true;
                }
            }

            if ((limit == -1 || limit >= vals.size() / 2) && ImGui::Selectable(ICON_FA_FILE " Add"))
            {
                if (!vals.is_array())
                {
                    vals.make_array(2);
                    value.set_item(vals);
                }

                vals.push_back(0.f);
                vals.push_back(0.f);
                modified = true;
            }
        }
        ImGui::EndChildFrame();

        return modified;
    }
};





JsonEdit::JsonEdit()
{
    add<JsonTypeColor>();
    add<JsonTypePoint>();
    add<JsonTypePoints>();
}

JsonEdit::~JsonEdit()
{
    auto* p = _types;
    while (p)
    {
        auto *pp = p->_next;
        delete p;
        p = pp;
    }
}

bool JsonEdit::show(msg::Var &data, const char *title)
{
    if (!data.is_object())
    {
        data.make_object(1);
    }

    JsonVal val{};
    val.p = data;
    val.n = 0xffffffff;

    return show_schema(_schema, val, title);
}

void JsonEdit::schema(const char *sch)
{
    _schema.from_string(sch);
}

void JsonEdit::schema(msg::Var &sch)
{
    _schema = sch;
}


void JsonEdit::next_open()
{
    _next_open = true;
}

bool JsonEdit::show_schema(msg::Var &sch, JsonVal &val, std::string_view key)
{
    bool modified = false;
    auto type_el = sch.get_item("type");
    auto title_el = sch.get_item("title");
    auto type = type_el.str();
    auto title = title_el.get(key);

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
    else if (type == "string")
    {
        auto def_el = sch.get_item("default");
        bool ret = false;
        _buffer = val.get_item().get(def_el.str());
        if (sch.get_item("format").str() == "multiline")
        {
            auto height = ImGui::GetTextLineHeight() * sch.get_item("lines").get(4);
            ret = ImGui::InputTextMultiline(title.data(), &_buffer, {-1, height});
        }
        else
        {
            ret = ImGui::InputText(title.data(), &_buffer);
        }

        if (ret)
        {
            val.set_item(_buffer);
            modified = true;
        }
    }
    else if (type == "number")
    {
        auto def_el = sch.get_item("default");
        float value = val.get_item().get(def_el.get(0.0f));
        bool ret = false;

        if (sch.get_item("format").str() == "slider")
            ret = ImGui::DragFloat(title.data(), &value);
        else
            ret = ImGui::InputFloat(title.data(), &value);

        if (ret)
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
        auto def_el = sch.get_item("default");
        int value = val.get_item().get(def_el.get(0));
        bool ret = false;

        if (sch.get_item("format").str() == "slider")
            ret = ImGui::DragInt(title.data(), &value);
        else
            ret = ImGui::InputInt(title.data(), &value);

        if (ret)
        {
            val.set_item(std::clamp(value,
                                    sch.get_item("minimum").get(INT_MIN),
                                    sch.get_item("maximum").get(INT_MAX)));
            modified = true;
        }
    }
    else if (type == "boolean")
    {
        auto def_el = sch.get_item("default");
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
            modified |= show_array(sch, val, title);
        }
        else if (type == "object")
        {
            modified |= show_object(sch, val, title);
        }
        else
        {
            for (auto p = _types; p; p = p->_next)
            {
                if (p->_name == type)
                {
                    modified |= p->ImguiProps(sch, val, title);
                    break;
                }
            }
        }

        return modified;
    }

    auto descr_el = sch.get_item("description");
    auto descr = descr_el.str();

    if (!descr.empty())
        ImGui::Text(descr.data());

    auto help_el = sch.get_item("help");
    auto help = help_el.str();
    if (!help.empty())
    {
        ImGui::SameLine();
        ImGui::HelpMarker(descr.data());
    }
    return modified;
}

bool JsonEdit::show_object(msg::Var &sch, JsonVal &val, std::string_view key)
{
    bool modified = false;
    if (!val.get_item().is_object())
    {
        fin::msg::Var obj;
        obj.make_object(1);
        val.set_item(obj);
    }

    auto flag = _next_open ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    _next_open = false;
    if (ImGui::CollapsingHeader(key.data(), flag))
    {
        auto descr_el = sch.get_item("description");
        auto descr = descr_el.str();
        if (!descr.empty())
            ImGui::Text(descr.data());

        ImGui::Indent();
        auto item = val.get_item();
        auto sch_items = sch["properties"];
        if (sch_items.is_object())
        {
            bool modified = false;
            JsonVal val;
            val.p = item;

            for (auto el : sch_items.members())
            {
                ImGui::PushID(val.n);
                val.k = el.first.str();
                if (show_schema(el.second, val, val.k))
                    modified = true;
                ImGui::PopID();
                ++val.n;
            }
            ImGui::Unindent();
            return false;
        }

        ImGui::Unindent();
    }

    return modified;
}

bool JsonEdit::show_array(msg::Var &sch, JsonVal &val, std::string_view key)
{
    bool modified = false;

    if (!val.get_item().is_array())
    {
        fin::msg::Var arr;
        arr.make_array(1);
        val.set_item(arr);
    }

    auto flag = _next_open ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    _next_open = false;
    if (ImGui::CollapsingHeader(key.data(), flag))
    {
        auto descr_el = sch.get_item("description");
        auto descr = descr_el.str();
        if (!descr.empty())
            ImGui::Text(descr.data());

        ImGui::Indent();
        auto sch_items = sch["items"];
        auto data_key = val.get_item();

        if (sch_items.is_object())
        {
            bool modified = false;
            JsonVal val{};
            val.p = data_key;
            int del = -1;

            for (auto el : data_key.elements())
            {
                ImGui::PushID(val.n);
                if (show_schema(sch_items, val, ImGui::FormatStr("%d", val.n)))
                    modified = true;
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_TRASH))
                    del = val.n;

                ImGui::PopID();
                ++val.n;
            }

            if (del >= 0)
            {
                data_key.erase(del);
                modified = true;
            }

            if (ImGui::Button(ICON_FA_FILE " Add"))
            {
                data_key.push_back(nullptr);
                modified = true;
            }

        }
        ImGui::Unindent();
    }

    return modified;
}

JsonType::JsonType(std::string_view id) : _name(id)
{
}


} // namespace fin
