
#include "utils/dialog_utils.hpp"
#include "imgui_utils.hpp"
#include "misc/cpp/imgui_stdlib.cpp"

namespace ImGui
{
const char *FormatStr(const char *fmt, ...)
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

static void *s_drag_data = 0;

void SetDragData(void *d)
{
    s_drag_data = d;
}

void *GetDragData()
{
    return s_drag_data;
}

bool InputJsonSchema(const fin::msg::Var &schema, fin::msg::Var &data)
{
    bool modified = false;

    for (auto& el : schema.members())
    {
        auto tp = el.second["type"];
        auto type = tp.str();
        auto key = el.first.str();

        if (type == "string")
        {
            std::string str_value = data[key].get("");
            if (ImGui::InputText(key.data(), &str_value))
            {
                data[key] = str_value;
                modified = true;
            }
        }
        else if (type == "number")
        {
            float num_value = data[key].get(0.0f);
            if (ImGui::InputFloat(key.data(), &num_value))
            {
                data[key] = num_value;
                modified = true;
            }
        }
        else if (type == "integer")
        {
            int int_value = data[key].get(0);
            if (ImGui::InputInt(key.data(), &int_value))
            {
                data[key] = int_value;
                modified = true;
            }
        }
        else if (type == "boolean")
        {
            bool bool_value = data[key].get(false);
            if (ImGui::Checkbox(key.data(), &bool_value))
            {
                data[key] = bool_value;
                modified = true;
            }
        }
        else if (type == "array" && el.second.contains("items"))
        {
            if (!data[key].is_array())
            {
                fin::msg::Var arr;
                arr.make_array(1);
                data[key] = arr;
            }

            auto data_key = data[key];
            ImGui::Text("Array:");
            ImGui::Indent();
            for (size_t i = 0; i < data_key.size(); ++i)
            {
                ImGui::PushID(i);
                auto item = data_key[i];
                modified |= InputJsonSchema(el.second["items"], item);
                if (ImGui::Button("Remove"))
                {
                    data_key.erase(i);
                    modified = true;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add"))
            {
                fin::msg::Var obj;
                obj.make_object(1);
                data_key.push_back(obj);
                modified = true;
            }
            ImGui::Unindent();
        }
        else if (type == "object" && el.second.contains("properties"))
        {
            if (!data[key].is_object())
            {
                fin::msg::Var obj;
                obj.make_object(1);
                data[key] = obj;
            }

            ImGui::Text("Object:");
            ImGui::Indent();
            auto item = data[key];
            modified |= InputJsonSchema(el.second["properties"], item);
            ImGui::Unindent();
        }
    }
    return modified;
}

bool OpenFileInput(const char *label, std::string &path, const char *filter)
{
    bool ret{};
    if (BeginCombo(label, path.c_str()))
    {
        auto r = fin::open_file_dialog("Open", path, fin::create_file_filter(filter));
        if (!r.empty())
        {
            ret = true;
            path = r[0];
        }
        ImGui::CloseCurrentPopup();
        ImGui::EndCombo();
    }
    return ret;
}

bool SaveFileInput(const char *label, std::string &path, const char *filter)
{
    bool ret{};
    if (BeginCombo(label, path.c_str()))
    {
        auto r = fin::save_file_dialog("Open", path, fin::create_file_filter(filter));
        if (!r.empty())
        {
            ret = true;
            path = r;
        }
        ImGui::CloseCurrentPopup();
        ImGui::EndCombo();
    }
    return ret;
}

}
