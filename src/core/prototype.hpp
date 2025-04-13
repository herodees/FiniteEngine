#pragma once

#include <core/editor/json_editor.hpp>
#include <core/atlas.hpp>

namespace fin
{

constexpr std::string_view SceneObjId("$scn");
constexpr std::string_view ObjId("$id");
constexpr std::string_view ObjUid("$uid");

class PrototypeRegister
{
public:
    PrototypeRegister();
    ~PrototypeRegister();

    static PrototypeRegister &instance();
    msg::Var prototype(uint64_t uid) const;

    bool show_props();
    bool show_workspace();
    bool show_menu();

    void scroll_to_center();
private:
    bool show_proto_props();

    std::vector<std::pair<std::string, msg::Var>> _prototypes;
    std::vector<msg::Var> _data;
    std::unordered_map<uint64_t, msg::Var> _proto;

    bool _scroll_to_center = true;
    bool _edit_origin = false;
    bool _edit_collision = false;
    JsonEdit _json_edit;
    size_t _active_tab = -1;
    int32_t _active_item = -1;
    size_t _prev_active_tab = -1;
    std::shared_ptr<Atlas> _atlas_ptr;
};

} // namespace fin
