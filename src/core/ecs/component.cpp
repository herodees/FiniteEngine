#include "component.hpp"
#include "base.hpp"
#include <core/scene.hpp>
#include <core/editor/imgui_control.hpp>
#include <utils/imguiline.hpp>

namespace fin
{
    const char GamePrefabFile[] = "game.prefab";

    struct FileDir
    {
        std::string                    path;
        std::vector<std::string>       dirs;
        std::vector<std::string>       files;
        std::shared_ptr<ImGui::Editor> editor;
        bool                           expanded{};
    };

    FileDir       _s_file_dir;
    std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> _s_cache;

    void reset_atlas_cache()
    {
        _s_cache.clear();
    }

    Atlas::Pack load_atlas(msg::Var& el)
    {
        Atlas::Pack out;
        auto        atl = el.get_item("src");
        auto        spr = el.get_item("spr");
        auto        it  = _s_cache.find(atl.str());
        if (it == _s_cache.end())
        {
            out                 = Atlas::load_shared(atl.str(), spr.str());
            _s_cache[atl.c_str()] = out.atlas;
        }
        else
        {
            out.atlas = it->second;
            if (auto n = out.atlas->find_sprite(spr.str()))
                out.sprite = &out.atlas->get(n);
        }
        return out;
    }

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

    void ComponentFactory::set_root(const std::string& startPath)
    {
        _base_folder        = startPath;
        _s_file_dir.expanded = false;
        _s_file_dir.path     = startPath;
    }

    bool ComponentFactory::load()
    {
        auto* txt = LoadFileText((_base_folder + GamePrefabFile).c_str());
        if (!txt)
        {
            SaveFileText((_base_folder + GamePrefabFile).c_str(), "{}");
            return false;
        }
        msg::Var doc;
        bool     r = doc.from_string(txt) == msg::VarError::ok;
        UnloadFileText(txt);
        if (!r)
            return false;

        return load(doc);
    }

    bool ComponentFactory::load(msg::Var& ar)
    {
        auto itm = ar.get_item("items");
        _prefabs = itm;
        _prefab_map.clear();
        for (auto el : _prefabs.elements())
        {
            auto key = el.get_item(Sc::Uid).get(0ull);
            _prefab_map[key] = el;
        }
        generate_prefab_map();
        return true;
    }

    bool ComponentFactory::save(msg::Var& ar)
    {
        ar.set_item("items", _prefabs);
        return true;
    }

    bool ComponentFactory::save()
    {
        msg::Var doc;
        if (!save(doc))
            return false;

        std::string str;
        doc.to_string(str);
        return SaveFileText((_base_folder + GamePrefabFile).c_str(), str.c_str());
    }

    void ComponentFactory::imgui_show(Scene* scene)
    {
    }

    bool ComponentFactory::imgui_prefab(Scene* scene, Entity edit)
    {
        bool ret = false;

        for (auto& component : _components)
        {
            if (component.second.contains(edit))
            {
                bool show = true;
                if (ImGui::CollapsingHeader(component.second.label.data(),
                                            &show,
                                            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ret |= component.second.edit(_registry, edit);
                }
                if (!show)
                {
                    component.second.remove(edit);
                    ret = true;
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
                if (ImGui::MenuItem(value.label.data(), 0, false, !value.contains(edit)))
                {
                    value.emplace(edit);
                    ret = true;
                }
            }
            ImGui::EndPopup();
        }
        return ret;
    }

    void ComponentFactory::imgui_props(Scene* scene)
    {
        if (!_prefab_explorer)
            return;
        if ((uint32_t)_selected >= _prefabs.size())
            return;
        if (_edit == entt::null)
            return;
        if (ecs::Base::contains(_edit))
        {
            auto obj = ecs::Base::get(_edit);
            if (obj->_layer)
            {
                _edit = entt::null;
            }
        }

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

        imgui_prefab(scene, _edit);
    }

    void ComponentFactory::imgui_prefabs(Scene* scene)
    {
        for (auto& [groupName, indices] : _groups)
        {
            if (groupName.empty() || ImGui::TreeNode(groupName.c_str()))
            {
                for (int n : indices)
                {
                    ImGui::PushID(n);

                    auto val  = _prefabs.get_item(n);
                    auto id   = val.get_item(Sc::Id);

                    if (ImGui::Selectable("##id", _selected == n, 0, {0, 25}))
                    {
                        selet_prefab(n);
                    }

                    if (ImGui::IsItemVisible())
                    {
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                        {
                            if (_edit == entt::null || _selected != n)
                            {
                                selet_prefab(n);
                            }
     
                            ImGui::SetDragDropPayload("ENTITY", &_edit, sizeof(Entity));
                            ImGui::EndDragDropSource();
                        }

                        auto spr  = val.get_item(ecs::Sprite::_s_id);
                        auto pack = load_atlas(spr);
                        if (pack.sprite)
                        {
                            ImGui::SameLine();
                            ImGui::SpriteImage(pack.sprite, {25, 25});
                        }
                        ImGui::SameLine();
                        ImGui::Text(id.c_str());
                    }

                    ImGui::PopID();
                }

                if (!groupName.empty())
                    ImGui::TreePop();
            }
        }
    }

    inline std::string_view get_file_icon(std::string_view file, uint32_t& clr)
    {
        clr    = 0xff808080;
        auto p = file.rfind(".");
        if (p == std::string_view::npos)
            return ICON_FA_FILE_PEN;
        auto ext = file.substr(p + 1);
        if (ext == "ogg" || ext == "wav")
        {
            clr = 0xff0051f2;
            return ICON_FA_FILE_AUDIO;
        }
        if (ext == "png" || ext == "jpg" || ext == "gif")
        {
            clr = 0xff60d71e;
            return ICON_FA_FILE_IMAGE;
        }
        if (ext == "atlas")
        {
            clr = 0xffcc6f98;
            return ICON_FA_FILE_ZIPPER;
        }
        if (ext == "prefab")
        {
            clr = 0xfff0f0f0;
            return ICON_FA_FILE_CODE;
        }
        return ICON_FA_FILE;
    }

    void ComponentFactory::imgui_explorer(Scene* scene)
    {
        reset_atlas_cache();

        if (!_s_file_dir.expanded)
        {
            _s_file_dir.expanded = true;
            _s_file_dir.dirs.clear();
            _s_file_dir.files.clear();
            if (_s_file_dir.path != _base_folder)
            {
                _s_file_dir.dirs.push_back("..");
            }

            for (const auto& entry : std::filesystem::directory_iterator(_s_file_dir.path))
            {
                if (entry.is_directory())
                {
                    _s_file_dir.dirs.push_back(entry.path().filename().string());
                }
                else if (entry.is_regular_file())
                {
                    _s_file_dir.files.push_back(entry.path().filename().string());
                }
            }
        }

        if (_s_file_dir.editor)
        {
            if (ImGui::LineSelect(ImGui::GetID(".."), false)
                    .Space()
                    .PushColor(0xff52d1ff)
                    .Text(ICON_FA_FOLDER)
                    .PopColor()
                    .Space()
                    .Text("..")
                    .End() ||
                !_s_file_dir.editor->imgui_show())
            {
                _s_file_dir.editor.reset();
            }
        }
        else
        {
            for (auto& dir : _s_file_dir.dirs)
            {
                if (ImGui::LineSelect(ImGui::GetID(dir.c_str()), false)
                        .Space()
                        .PushColor(0xff52d1ff)
                        .Text(ICON_FA_FOLDER)
                        .PopColor()
                        .Space()
                        .Text(dir.c_str())
                        .End())
                {
                    _s_file_dir.expanded = false;
                    if (dir == "..")
                    {
                        _s_file_dir.path.pop_back();
                        _s_file_dir.path.resize(_s_file_dir.path.rfind('/') + 1);
                    }
                    else
                    {
                        _s_file_dir.path += dir;
                        _s_file_dir.path += '/';
                    }
                }
            }

            for (auto& file : _s_file_dir.files)
            {
                uint32_t clr;
                auto     ico = get_file_icon(file, clr);

                if (ImGui::LineSelect(ImGui::GetID(file.c_str()), false)
                        .Space()
                        .PushColor(clr)
                        .Text(ico)
                        .PopColor()
                        .Space()
                        .Text(file.c_str())
                        .End())
                {
                    _s_file_dir.editor = ImGui::Editor::load_from_file(_s_file_dir.path + file);
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    std::string_view ext(file);
                    ext = ext.substr(ext.rfind('.') + 1);
                    if (ext == "map")
                    {
                        scene->load(_s_file_dir.path + file);
                    }
                }
            }
        }
    }

    void ComponentFactory::imgui_items(Scene* scene)
    {
        ImGui::LineItem("astmnu", {-1, ImGui::GetFrameHeight()})
            .Space()
            .PushStyle(ImStyle_Button, 1, !_prefab_explorer)
            .Text(ICON_FA_FOLDER_TREE)
            .PopStyle()
            .PushStyle(ImStyle_Button, 2, _prefab_explorer)
            .Text(ICON_FA_BOX_ARCHIVE)
            .PopStyle()
            .Space(8);

        if (_prefab_explorer)
        {
            ImGui::Line()
                .Spring()
                .PushStyle(ImStyle_Button, 10)
                .Text(ICON_FA_FILE_CIRCLE_PLUS)
                .PopStyle()
                .PushStyle(ImStyle_Button, 11)
                .Text(ICON_FA_FLOPPY_DISK)
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 20)
                .Text(ICON_FA_TRASH)
                .PopStyle();
        }

        if (ImGui::Line().End())
        {
            switch (ImGui::Line().HoverId())
            {
                case 1:
                    _prefab_explorer = false;
                    break;
                case 2:
                    _prefab_explorer = true;
                    break;
                case 10:
                {
                    auto obj = create_empty_prefab(ImGui::FormatStr("%s_%d", "Entity", _prefabs.size()), "");

                    int n = _prefabs.size();
                    _prefabs.push_back(obj);
                    _prefab_map[obj.get_item(Sc::Uid).get(0ull)] = obj;
                    _groups[""].push_back(n);
                    selet_prefab(n);
                }
                    break;
                case 11:
                    selet_prefab(_selected);
                    save();
                    break;
                case 20:
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
                    break;
            }

        }

        if (ImGui::BeginChildFrame(ImGui::GetID("pfitms"), {-1, -1}))
        {
            if (_prefab_explorer)
            {
                imgui_prefabs(scene);
            }
            else
            {
                imgui_explorer(scene);
            }
        }
        ImGui::EndChildFrame();
    }


} // namespace fin
