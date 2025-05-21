#include "component.hpp"

#include <core/scene.hpp>
#include <core/editor/imgui_control.hpp>
#include <utils/imguiline.hpp>

namespace fin
{
    Entity ComponentFactory::get_old_entity(Entity old_id)
    {
        if (old_id == entt::null)
            return entt::null;

        if (!_entity_map.contains(old_id))
        {
            auto e = _registry.create();
            _entity_map.emplace(old_id, e);
            return e;
        }

        return _entity_map.get(old_id);
    }

    void ComponentFactory::serialize(msg::Var& ar)
    {
        auto& reg = _registry;
        auto& cmp = _components;

        _registry.each(
            [&ar, &cmp, &reg](auto entity)
            {
                msg::Var data;
                data.set_item("$id", static_cast<uint32_t>(entity));
                for (auto& [key, value] : cmp)
                {
                    ArchiveParams ap{reg, entity};
                    if (!value.save(ap))
                    {
                        TraceLog(LOG_WARNING, "Component %.*s not saved", key.size(), key.data());
                    }
                    data.set_item(key, ap.data);
                }
                ar.push_back(data);
            });
    }

    void ComponentFactory::deserialize(msg::Var& ar)
    {
        _registry.clear();
        _entity_map.clear();

        for (auto& data : ar.elements())
        {
            auto id     = static_cast<Entity>(data["$id"].get(static_cast<uint32_t>(entt::null)));
            auto entity = get_old_entity(id);

            load_prefab(entity, data);
        }
    }

    void ComponentFactory::load_prefab(Entity entity, msg::Var& data)
    {
        for (auto e : data.members())
        {
            auto c = e.first.str();
            if (c[0] == '$')
                continue;

            auto it = _components.find(c);
            if (it == _components.end())
            {
                TraceLog(LOG_WARNING, "Component %.*s not registered", c.size(), c.data());
                continue;
            }

            if (!it->second.contains(entity))
            {
                it->second.emplace(entity);
            }

            ArchiveParams ap{_registry, entity, e.second};
            if (!it->second.load(ap))
            {
                TraceLog(LOG_WARNING, "Component %.*s not loaded", c.size(), c.data());
            }
        }
    }

    void ComponentFactory::save_prefab(Entity e, msg::Var& ar)
    {
        for (auto& cmp : _components)
        {
            if (cmp.second.contains(e))
            {
                ArchiveParams ap{_registry, e};
                if (!cmp.second.save(ap))
                {
                    TraceLog(LOG_WARNING, "Component %.*s not saved", cmp.first.size(), cmp.first.data());
                }
                ar.set_item(cmp.first, ap.data);
            }
            else
            {
                ar.erase(cmp.first);
            }
        }
    }

    void ComponentFactory::selet_prefab(int32_t n)
    {
        if (_edit != entt::null)
        {
            if ((uint32_t)_selected < _prefabs.size())
            {
                auto pr = _prefabs[_selected];
                save_prefab(_edit, pr);
            }
            _registry.destroy(_edit);
            _edit = entt::null;
        }
        _selected = n;
        if ((uint32_t)_selected >= _prefabs.size())
            return;

        _edit = _registry.create();
        auto el = _prefabs.get_item(n);
        load_prefab(_edit, el);
    }

    void ComponentFactory::generate_prefab_map()
    {
        _groups.clear();
        for (int i = 0; i < _prefabs.size(); i++)
        {
            auto val   = _prefabs.get_item(i);
            auto group = val.get_item(Sc::Group); // assuming std::string
            _groups[group.get("")].push_back(i);
        }
    }

    msg::Var ComponentFactory::create_empty_prefab(std::string_view name, std::string_view group)
    {
        msg::Var obj;
        obj.set_item(Sc::Id, name);
        obj.set_item(Sc::Group, group);
        obj.set_item(Sc::Uid, std::generate_unique_id());
        return obj;
    }


    void ComponentFactory::imgui_show(Scene* scene)
    {
    }

    void ComponentFactory::imgui_props(Scene* scene)
    {
        if ((uint32_t)_selected >= _prefabs.size())
            return;
        if (_edit == entt::null)
            return;

        auto proto = _prefabs.get_item(_selected);
        _buff      = proto.get_item(Sc::Id).str();
        if (ImGui::InputText("Id", &_buff))
        {
            proto.set_item(Sc::Id, _buff);
        }
        _buff = proto.get_item(Sc::Group).str();
        if (ImGui::InputText("Group", &_buff))
        {
            proto.set_item(Sc::Group, _buff);
            generate_prefab_map();
        }

        for (auto& component : _components)
        {
            if (component.second.contains(_edit))
            {
                bool show = true;
                if (ImGui::CollapsingHeader(component.second.label.data(), &show, 0))
                {
                }
                if (!show)
                {
                    component.second.remove(_edit);
                }
            }
        }

        ImGui::LineItem(ImGui::GetID("crbtn"), {-1, ImGui::GetFrameHeight()})
            .Spring()
            .PushStyle(ImStyle_Button, 1)
            .Space()
            .Text(ICON_FA_PLUS " New Component")
            .Space()
            .PopStyle()
            .Spring()
            .End();


        if (ImGui::Line().Return() && ImGui::Line().HoverId() == 1)
        {
            ImGui::OpenPopup("ComponentMenu");
        }

        if (ImGui::BeginPopup("ComponentMenu"))
        {
            for (auto& [key, value] : _components)
            {
                if (ImGui::MenuItem(value.label.data(), 0, false, !value.contains(_edit)))
                {
                    value.emplace(_edit);
                }
            }
            ImGui::EndPopup();
        }
    }

    void ComponentFactory::imgui_items(Scene* scene)
    {
        if (ImGui::Button(" " ICON_FA_LAPTOP " Add "))
        {
            auto obj = create_empty_prefab(ImGui::FormatStr("%s_%d", "Entity", _prefabs.size()), "");

            int n = _prefabs.size();
            _prefabs.push_back(obj);
            _prefab_map[obj.get_item(Sc::Uid).get(0ull)] = obj;
            _groups[""].push_back(n);
            selet_prefab(n);
        }


        ImGui::SameLine();
        if (ImGui::Button(" " ICON_FA_UPLOAD " Save "))
        {

        }


        ImGui::SameLine();
        if (ImGui::Button(" " ICON_FA_DOWNLOAD " "))
        {
            ImGui::OpenPopup("AtlasMenu");
        }


        ImGui::SameLine();
        ImGui::InvisibleButton("##ib", {-34, 1});
        ImGui::SameLine();
        if (ImGui::Button(" " ICON_FA_BAN " "))
        {
            if ((uint32_t)_selected < _prefabs.size())
            {
                auto t = _selected;
                selet_prefab(-1);
                _prefabs.erase(t);
                selet_prefab(t);
                generate_prefab_map();
            }
        }

        if (ImGui::BeginChildFrame(-1, {-1, -1}))
        {
            for (auto& [groupName, indices] : _groups)
            {
                if (groupName.empty() || ImGui::TreeNode(groupName.c_str()))
                {
                    for (int n : indices)
                    {
                        ImGui::PushID(n);
                        auto val = _prefabs.get_item(n);
                        auto id  = val.get_item(Sc::Id);

                        if (ImGui::Selectable(id.c_str(), _selected == n))
                        {
                            selet_prefab(n);
                        }

                        ImGui::PopID();
                    }

                    if (!groupName.empty())
                        ImGui::TreePop();
                }
            }
        }
        ImGui::EndChildFrame();
    }



} // namespace fin
