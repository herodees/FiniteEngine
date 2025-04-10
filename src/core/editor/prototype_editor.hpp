#pragma once

#include "json_editor.hpp"

namespace fin
{

class PrototypeEditor
{
public:
    PrototypeEditor();
    ~PrototypeEditor();

    bool show_props();
    bool show_workspace();
    bool show_menu();

    void scroll_to_center();

private:
    bool show_proto_props();

    std::vector<std::pair<std::string, msg::Var>> _prototypes;
    std::vector<msg::Var> _data;
    bool _scroll_to_center = true;
    bool _edit_origin = false;
    bool _edit_collision = false;
    JsonEdit _json_edit;
    size_t _active_tab = -1;
    int32_t _active_item = -1;
    size_t _prev_active_tab = -1;
};

} // namespace fin
