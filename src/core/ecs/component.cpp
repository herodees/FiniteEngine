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

    ImGui::CanvasParams                                                                        canvas;
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

    Registry& ComponentFactory::get_registry()
    {
        return _registry;
    }

    ComponentFactory::Map& ComponentFactory::get_components()
    {
        return _components;
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

    void ComponentFactory::clear_old_entities()
    {
        _entity_map.clear();
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
        msg::Var* target = &data;
        msg::Var  load;

        auto pfab = data.get_item(Sc::Uid);
        if (!pfab.is_undefined())
        {
            auto uid = pfab.get(0ull);
            auto it = _prefab_map.find(uid);
            if (it == _prefab_map.end())
            {
                TraceLog(LOG_WARNING, "Prefab with UID %llu not found", uid);
                return;
            }
            auto& cmp = ecs::Prefab::emplace_or_replace(entity);
            cmp._data = it->second;
            load      = it->second.clone();
            load.apply(data.get_item(Sc::Diff));
            target = &load;
        }

        for (auto e : target->members())
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
        if (ecs::Prefab::contains(e))
        {
            auto* base = ecs::Prefab::get(e);

            msg::Var arr;
            arr.set_item(Sc::Id, (uint32_t)e);
            for (auto& cmp : _components)
            {
                if (cmp.first == ecs::Prefab::_s_id)
                    continue;

                if (cmp.second.contains(e))
                {
                    ArchiveParams ap{_registry, e};
                    if (!cmp.second.save(ap))
                    {
                        TraceLog(LOG_WARNING, "Component %.*s not saved", cmp.first.size(), cmp.first.data());
                    }
                    arr.set_item(cmp.first, ap.data);
                }
                else
                {
                    arr.erase(cmp.first);
                }
            }

            msg::Var diff = base->_data.diff(arr);
            ar.set_item(Sc::Id, (uint32_t)e);
            ar.set_item(Sc::Uid, base->_data.get_item(Sc::Uid));
            ar.set_item(Sc::Diff, diff);
        }
        else
        {
            ar.set_item(Sc::Id, (uint32_t)e);
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
    }

    void ComponentFactory::selet_prefab(int32_t n)
    {
        if (_edit != entt::null)
        {
            _registry.destroy(_edit);
            _edit = entt::null;
        }
        _selected = n;
        if ((uint32_t)_selected >= _prefabs.size())
            return;

        _edit = _registry.create();
        auto el = _prefabs.get_item(n);
        load_prefab(_edit, el);

        auto& prefab = ecs::Prefab::emplace_or_replace(_edit);
        prefab._data = el;
    }

    void ComponentFactory::edit_prefab(int32_t n)
    {
        if (_prefab_edit != entt::null)
            return;

        _prefab_edit_index = n;
        auto el            = _prefabs.get_item(n);
        _prefab_edit       = _registry.create();
        load_prefab(_prefab_edit, el);
    }

    void ComponentFactory::save_edit_prefab(Scene* scene)
    {
        if (_prefab_edit == entt::null)
            return;

        uint64_t uid = _prefabs.get_item(_prefab_edit_index).get_item(Sc::Uid).get(0ull);

        msg::Var data;
        data.set_item(Sc::Id, _prefabs.get_item(_prefab_edit_index).get_item(Sc::Id).clone());
        data.set_item(Sc::Group, _prefabs.get_item(_prefab_edit_index).get_item(Sc::Group).clone());
        data.set_item(Sc::Uid, uid);

        save_prefab(_prefab_edit, data);

        _prefabs.set_item(_prefab_edit_index, data);
    }

    void ComponentFactory::close_edit_prefab(Scene* scene)
    {
        if (_prefab_edit == entt::null)
            return;
        scene->factory().get_registry().destroy(_prefab_edit);
        _prefab_edit = entt::null;
        _prefab_edit_index = -1;
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
        obj.set_item("_", nullptr);
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

    void ComponentFactory::load_prefab(Entity e, msg::Var& prefab, msg::Var& diff)
    {
    }

    void ComponentFactory::save_prefab(Entity e, msg::Var& prefab, msg::Var& diff)
    {
    }

    bool ComponentFactory::imgui_menu(Scene* scene)
    {
        ImGui::LineItem(ImGui::GetID("prefab"), {-1, ImGui::GetFrameHeight()}).Space();

        int32_t n = 1;
        for (auto& cmp : _components)
        {
            if (cmp.second.contains(_prefab_edit))
            {
                ImGui::Line()
                    .PushStyle(ImStyle_Header, n, _selected_component == &cmp.second)
                    .Space()
                    .Text(cmp.second.label.data())
                    .Space()
                    .PopStyle();
            }
            ++n;
        }

        ImGui::Line()
            .Spring()
            .PushStyle(ImStyle_Header, -5)
            .Space()
            .Text(ICON_FA_ARROWS_TO_CIRCLE)
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Header, -4)
            .Format("%.1f%%", canvas.zoom * 100)
            .PopStyle()
            .Space(16)
            .PushStyle(ImStyle_Header, -3)
            .Space()
            .Text(ICON_FA_FLOPPY_DISK " Save")
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Header, -2)
            .Space()
            .Text("Close")
            .Space()
            .PopStyle()
            .Space();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == -5)
            {
                canvas.CenterOrigin();
            }
            else if(ImGui::Line().HoverId() == -4)
            {
                canvas.zoom = 1.0f;
            }
            else if(ImGui::Line().HoverId() < 0)
            {
                if (ImGui::Line().HoverId() == -3)
                {
                    save_edit_prefab(scene);
                }
                scene->set_mode(SceneMode::Scene);
                close_edit_prefab(scene);
            }
            else
            {
                auto it = _components.begin();
                std::advance(it, ImGui::Line().HoverId() - 1);
                _selected_component = &it->second;
            }
        }
        return true;
    }

    bool ComponentFactory::imgui_prefab(Scene* scene, Entity edit)
    {
        bool ret = false;

        for (auto& component : _components)
        {
            if (component.second.contains(edit))
            {
                bool show = true;
                bool change = false;
                if (ImGui::CollapsingHeader(component.second.label.data(),
                                            component.first != "_" ? & show : nullptr,
                                            ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::PushID(component.first.data());
                    change = component.second.edit(edit);
                    ImGui::PopID();
                }

                if (change)
                {
                    ret |= change;
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
        auto shop_prefab_names = [&](int ent)
            {
                auto proto = _prefabs.get_item(ent);
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
            };
    

        if (_prefab_edit != entt::null)
        {
            shop_prefab_names(_prefab_edit_index);
            imgui_prefab(scene, _prefab_edit);
            return;
        }

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

        shop_prefab_names(_selected);
        ImGui::BeginDisabled(true);
        imgui_prefab(scene, _edit);
        ImGui::EndDisabled();
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

                    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
                    {
                        scene->set_mode(SceneMode::Prefab);
                        edit_prefab(n);
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
            .Space()
            .Text(ICON_FA_FOLDER_TREE)
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Button, 2, _prefab_explorer)
            .Space()
            .Text(ICON_FA_BOX_ARCHIVE)
            .Space()
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
                        if (_prefab_edit == entt::null)
                        {
                            auto t = _selected;
                            selet_prefab(-1);
                            _prefabs.erase(t);
                            selet_prefab(t);
                            generate_prefab_map();
                        }
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

    bool ComponentFactory::imgui_workspace(Scene* scene)
    {
        static bool init_canvas = true;

        if (ImGui::BeginCanvas("SceneCanvas", ImVec2(0, 0), canvas))
        {
            if (init_canvas)
            {
                canvas.CenterOrigin();
                init_canvas = false;
            }

            ImGui::DrawGrid(canvas);
            ImGui::DrawOrigin(canvas, -1);

            if (_prefab_edit != entt::null && ecs::Base::contains(_prefab_edit))
            {
                auto* base = ecs::Base::get(_prefab_edit);
                if (ecs::Sprite::contains(_prefab_edit))
                {
                    auto& spr = ecs::Sprite::get(_prefab_edit)->pack;
                    if (spr.sprite)
                    {
                        Region<float> reg( -spr.sprite->_origina.x,
                                           -spr.sprite->_origina.y,
                                           spr.sprite->_source.width - spr.sprite->_origina.x,
                                           spr.sprite->_source.height - spr.sprite->_origina.y);

                        ImVec2 txs(spr.sprite->_texture->width, spr.sprite->_texture->height);

                        ImGui::DrawTexture(canvas,
                                           (ImTextureID)spr.sprite->_texture,
                                           {reg.x1, reg.y1},
                                           {reg.x2, reg.y2},
                                           {spr.sprite->_source.x / txs.x, spr.sprite->_source.y / txs.y},
                                           {spr.sprite->_source.x2() / txs.x, spr.sprite->_source.y2() / txs.y}, 0xffffffff);


                        ImGui::GetWindowDrawList()->AddRect(canvas.WorldToScreen({reg.x1, reg.y1}), canvas.WorldToScreen({reg.x2, reg.y2}),0xffffffff);
                    }
                }

                if (_selected_component)
                {
                    if (_selected_component->contains(_prefab_edit))
                    {
                        ImGui::PushID(_selected_component);
                        _selected_component->edit_canvas(canvas, _prefab_edit);
                        ImGui::PopID();
                    }
                }

            }

            ImGui::DrawRuler(canvas);
            ImGui::EndCanvas();
        }

        return true;
    }


} // namespace fin
