#include "component.hpp"
#include "base.hpp"
#include <core/scene.hpp>
#include <core/editor/imgui_control.hpp>
#include <utils/imguiline.hpp>
#include <utils/dialog_utils.hpp>

namespace fin
{
    const char GamePrefabFile[] = "game.prefab";

    static msg::Var create_empty_prefab(std::string_view name, std::string_view group)
    {
        msg::Var obj;
        msg::Var cls;
        cls.set_item("_", nullptr);
        obj.set_item(Sc::Uid, std::generate_unique_id());
        obj.set_item(Sc::Id, name);
        obj.set_item(Sc::Group, group);
        obj.set_item(Sc::Class, cls);
        return obj;
    }

    struct FileDir
    {
        std::string                    path;
        std::vector<std::string>       dirs;
        std::vector<std::string>       files;
        std::shared_ptr<ImGui::Editor> editor;
        bool                           expanded{};
    };

    static ImGui::CanvasParams                                                                        _s_canvas;
    static FileDir                                                                                    _s_file_dir;
    static std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> _s_cache;
    static msg::Var                                                                                   _s_copy;
    static ComponentData*                                                                             _s_comp{};




    class ImportDialog : public ImGui::Dialog
    {
    public:
        ImportDialog(ComponentFactory& factory) : _factory(factory)
        {
            _components.resize(_factory.get_components().size());
        }

        bool OnImport()
        {
            if (!_atlas)
                return false;

            for (auto i = 0; i < _atlas->size(); ++i)
            {
                auto& spr = _atlas->get(i + 1);
                auto  obj = create_empty_prefab(spr._name, _group);
                auto  cls = obj.get_item(Sc::Class);

                auto&  cmps = _factory.get_components();
                size_t n    = 0;
                for (auto& el : cmps)
                {
                    if (_components[n++])
                    {
                        if (el.first == ecs::Sprite::_s_id)
                        {
                            msg::Var ar;
                            ar.set_item("src", _atlas.get()->get_path());
                            ar.set_item("spr", spr._name);
                            cls.set_item(el.first, ar);
                        }
                        else
                        {
                            cls.set_item(el.first, nullptr);
                        }
                    }
                }
                _factory.push_prefab_data(obj);
            }
            _factory.generate_prefab_map();
            return true;
        }

        bool OnUpdate() override
        {
            if (ImGui::OpenFileName("Sprite Atlas", _path, ".atlas"))
            {
                _atlas = Atlas::load_shared(_path);
            }
            if (ImGui::InputText("Group", &_group))
            {
            }
            if (ImGui::BeginCombo("Components", "Select Component", 0))
            {
                auto&  cmps = _factory.get_components();
                size_t n    = 0;
                for (auto& el : cmps)
                {
                    if (el.first == ecs::Sprite::_s_id || el.first == ecs::Base::_s_id)
                    {
                        _components[n] = true;
                    }
                    ImGui::Checkbox(el.second.label.data(), &(bool&)_components[n++]);
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginChildFrame(ImGui::GetID(this), {-1, -2 * ImGui::GetFrameHeightWithSpacing()}) && _atlas)
            {
                for (auto n = 0; n < _atlas->size(); ++n)
                {
                    auto& spr = _atlas->get(n+1);
                    ImGui::Selectable(spr._name.c_str());
                }
            }
            ImGui::EndChildFrame();

            ImGui::NewLine();
            ImGui::Separator();
            if (ImGui::Button("Cancel"))
                _valid = false;

            ImGui::SameLine();
            if (ImGui::Button("Import") && OnImport())
                _valid = false;

            return _valid;
        }

        ComponentFactory&    _factory;
        std::string          _path;
        std::string          _group;
        std::vector<uint8_t> _components;
        Atlas::Ptr           _atlas;
        bool                 _valid = true;
    };




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


    void ComponentFactory::load_entity(Entity& entity, msg::Var& data)
    {
        auto old_entity = (Entity)data[Sc::Id].get((uint32_t)entt::null); // Ensure Id is present
        entity          = get_old_entity(old_entity);
        if (entity == entt::null)
        {
            return; // If the entity is null, we cannot load it
        }

        auto uid = data[Sc::Uid];
        auto cls = data[Sc::Class];

        if (!uid.is_undefined())
        {
            auto nuid = uid.get(0ull);
            auto it   = _prefab_map.find(nuid);
            if (it == _prefab_map.end())
            {
                TraceLog(LOG_WARNING, "Prefab with UID %llu not found", nuid);
                return;
            }
            auto& cmp = ecs::Prefab::emplace_or_replace(entity);
            cmp._data = it->second;                             // Store the prefab data in the component
            auto load = it->second.get_item(Sc::Class).clone(); // Clone the prefab data
            load.apply(cls);                                    // Apply the class data to the cloned prefab data
            load_prefab_components(entity, load);               // Load the components from the class data
        }
        else
        {
            load_prefab_components(entity, cls); // Load the components from the class data
        }
    }

    void ComponentFactory::save_entity(Entity entity, msg::Var& data)
    {
        data.set_item(Sc::Id, (uint32_t)entity);
        msg::Var cls;
        if (ecs::Prefab::contains(entity))
        {
            auto* base = ecs::Prefab::get(entity);
            save_prefab_components(entity, cls);

            msg::Var diff = base->_data.get_item(Sc::Class).diff(cls);
            data.set_item(Sc::Uid, base->_data.get_item(Sc::Uid));
            data.set_item(Sc::Class, diff);
        }
        else
        {
            save_prefab_components(entity, cls);
            data.set_item(Sc::Class, cls);
        }
    }

    int ComponentFactory::push_prefab_data(msg::Var& obj)
    {
        int n = _prefabs.size();
        _prefabs.push_back(obj);
        _prefab_map[obj.get_item(Sc::Uid).get(0ull)] = obj;
        _groups[""].push_back(n);
        return n;
    }

    void ComponentFactory::load_prefab_components(Entity entity, msg::Var& data)
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

    void ComponentFactory::save_prefab_components(Entity entity, msg::Var& data)
    {
        data.make_object(_components.size());
        data.erase();
        for (auto& cmp : _components)
        {
            if (cmp.second.contains(entity))
            {
                ArchiveParams ap{_registry, entity};
                if (!cmp.second.save(ap))
                {
                    TraceLog(LOG_WARNING, "Component %.*s not saved", cmp.first.size(), cmp.first.data());
                }
                data.set_item(cmp.first, ap.data);
            }
        }
    }

    void ComponentFactory::selet_prefab(Scene* scene, int32_t n)
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
        auto cls = el.get_item(Sc::Class);
        load_prefab_components(_edit, cls);

        auto& prefab = ecs::Prefab::emplace_or_replace(_edit);
        prefab._data = el;
    }

    void ComponentFactory::edit_prefab(Scene* scene, int32_t n)
    {
        if (_prefab_edit != entt::null)
        {
            if (_prefab_changed && 1 ==
                messagebox_yes_no("Edit Prefab", "Do you want to save the current prefab before editing?"))
            {
                save_edit_prefab(scene);
            }
            close_edit_prefab(scene);
        }

        _prefab_edit_index = n;
        auto el            = _prefabs.get_item(n);
        auto cls           = el.get_item(Sc::Class);
        _prefab_edit       = _registry.create();
        load_prefab_components(_prefab_edit, cls);
        _prefab_changed = false;
    }

    void ComponentFactory::save_edit_prefab(Scene* scene)
    {
        if (_prefab_edit == entt::null)
            return;

        auto prefab = _prefabs.get_item(_prefab_edit_index);
        auto cls = prefab.get_item(Sc::Class);
        save_prefab_components(_prefab_edit, cls);

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

    bool ComponentFactory::imgui_menu(Scene* scene)
    {
        ImGui::LineItem(ImGui::GetID("prefab"), {-1, ImGui::GetFrameHeight()}).Space();

        int32_t n = 1;
        for (auto& cmp : _components)
        {
            if (cmp.second.HasWorkspaceEditor() && cmp.second.contains(_prefab_edit))
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
            .Format("%.1f%%", _s_canvas.zoom * 100)
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
                _s_canvas.CenterOrigin();
            }
            else if(ImGui::Line().HoverId() == -4)
            {
                _s_canvas.zoom = 1.0f;
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

    bool ComponentFactory::imgui_component(Scene* scene, ComponentData* comp, Entity entity)
    {
        bool ret = false;

        ImGui::LineItem(comp->id.data(), {-1, ImGui::GetFrameHeightWithSpacing()}).Expandable(true).Space();

        ImGui::PushID(comp->id.data());
        ImGui::Line()
            .PushStyleColor(ImGuiCol_ButtonActive)
            .Text(ImGui::Line().Expanded() ? ICON_FA_CARET_DOWN " " : ICON_FA_CARET_RIGHT " ")
            .Text(comp->label.data())
            .PopStyleColor()
            .Spring();

        if (comp->HasWorkspaceEditor())
        {
            ImGui::Line().PushStyle(ImStyle_Button, 2, _selected_component == comp).Space().Text(ICON_FA_PEN).Space().PopStyle();
        }

        ImGui::Line()
            .PushStyle(ImStyle_Button, 1)
            .Space()
            .Text(ICON_FA_GEAR)
            .Space()
            .PopStyle();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == 1)
                ImGui::OpenPopup("##settings_popup");
            else if (ImGui::Line().HoverId() == 2)
                _selected_component = comp; // Set the selected component for editing in the workspace
        }

        if (ImGui::BeginPopup("##settings_popup"))
        {
            if (ImGui::MenuItem("Reset"))
            {
                ArchiveParams ar{_registry, entity};
                comp->load(ar);
            }
            ImGui::Separator(); 
            if (ImGui::MenuItem("Remove Component", 0, false, comp->id != "_"))
            {
                comp->remove(entity);
                ret = true;
            }
            ImGui::Separator(); 
            if (ImGui::MenuItem("Copy Component"))
            {
                ArchiveParams ar{_registry, entity};
                comp->save(ar);
                _s_copy = ar.data; // Store the component data in a global variable for pasting later
                _s_comp = comp;    // Store the component pointer for pasting later
            }
            if (ImGui::MenuItem("Paste Component", 0, false, _s_comp && _s_copy.is_object()) && _s_comp)
            {
                if (!_s_comp->contains(entity)) // Ensure the component exists for the entity
                    _s_comp->emplace(entity);   // If not, create it
                ArchiveParams ar{_registry, entity, _s_copy};
                _s_comp->load(ar);
            }
            ImGui::EndPopup();
        }

        if (ImGui::Line().Expanded() && !ret && comp)
            ret = comp->edit(entity);

        ImGui::PopID();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));
        ImGui::Separator();
        ImGui::PopStyleColor();
        return ret;
    }

    bool ComponentFactory::imgui_prefab(Scene* scene, Entity edit)
    {
        bool ret = false;
        bool add = false;

        for (auto& component : _components)
        {
            if (component.second.contains(edit))
            {
                ret |= imgui_component(scene, &component.second, edit);
            }
        }

        ImGui::LineItem(ImGui::GetID("crbtn"), {-1, ImGui::GetFrameHeight()})
            .Spring()
            .PushStyle(ImStyle_Button, 1)
            .Space()
            .Text(ICON_FA_PLUS " Add Component")
            .Space()
            .PopStyle()
            .Spring()
            .End();

        if (ImGui::Line().Return() && ImGui::Line().HoverId() == 1)
        {
            add = true;
        }

        if (ImGui::BeginPopupContextWindow("BackgroundPopup", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_FA_PLUS " Add Component"))
                add = true;
            if (ImGui::MenuItem("Paste Component", 0, false, _s_comp && _s_copy.is_object()))
            {
                if (!_s_comp->contains(edit)) // Ensure the component exists for the entity
                    _s_comp->emplace(edit);   // If not, create it
                ArchiveParams ar{_registry, edit, _s_copy};
                _s_comp->load(ar);
            }
            ImGui::EndPopup();
        }

        if (add)
            ImGui::OpenPopup("ComponentMenu");
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
        auto show_prefab_names = [&](int ent)
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
            show_prefab_names(_prefab_edit_index);
            if (imgui_prefab(scene, _prefab_edit))
            {
                _prefab_changed = true;
            }
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

        show_prefab_names(_selected);
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
                        selet_prefab(scene, n);
                    }

                    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
                    {
                        scene->set_mode(SceneMode::Prefab);
                        edit_prefab(scene, n);
                    }

                    if (ImGui::IsItemVisible())
                    {
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                        {
                            if (_edit == entt::null || _selected != n)
                            {
                                selet_prefab(scene, n);
                            }
     
                            ImGui::SetDragDropPayload("ENTITY", &_edit, sizeof(Entity));
                            ImGui::EndDragDropSource();
                        }

                        auto spr  = val.get_item(Sc::Class).get_item(ecs::Sprite::_s_id);
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
            .Text(ICON_FA_FOLDER_TREE " Assets")
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Button, 2, _prefab_explorer)
            .Space()
            .Text(ICON_FA_BOX_ARCHIVE " Prefabs")
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
                .PushStyle(ImStyle_Button, 12)
                .Text(ICON_FA_FILE_IMPORT)
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
                    int n = push_prefab_data(obj);
                    selet_prefab(scene, n);
                }
                break;
                case 11:
                    selet_prefab(scene, _selected);
                    save();
                    break;
                case 12:
                    ImGui::Dialog::Show<ImportDialog>("Import Sprites", {800,600}, 0, *this);
                    break;
                case 20:
                {
                    if ((uint32_t)_selected < _prefabs.size())
                    {
                        if (_prefab_edit == entt::null)
                        {
                            auto t = _selected;
                            selet_prefab(scene, -1);
                            _prefabs.erase(t);
                            selet_prefab(scene, t);
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

        if (ImGui::BeginCanvas("SceneCanvas", ImVec2(0, 0), _s_canvas))
        {
            if (init_canvas)
            {
                _s_canvas.CenterOrigin();
                init_canvas = false;
            }

            ImGui::DrawGrid(_s_canvas);
            ImGui::DrawOrigin(_s_canvas, -1);

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

                        ImGui::DrawTexture(_s_canvas,
                                           (ImTextureID)spr.sprite->_texture,
                                           {reg.x1, reg.y1},
                                           {reg.x2, reg.y2},
                                           {spr.sprite->_source.x / txs.x, spr.sprite->_source.y / txs.y},
                                           {spr.sprite->_source.x2() / txs.x, spr.sprite->_source.y2() / txs.y}, 0xffffffff);


                        ImGui::GetWindowDrawList()->AddRect(_s_canvas.WorldToScreen({reg.x1, reg.y1}),
                                                            _s_canvas.WorldToScreen({reg.x2, reg.y2}),
                                                            0xffffffff);
                    }
                }

                if (_selected_component && _selected_component->HasWorkspaceEditor())
                {
                    if (_selected_component->contains(_prefab_edit))
                    {
                        ImGui::PushID(_selected_component);
                        _selected_component->edit_canvas(_s_canvas, _prefab_edit);
                        ImGui::PopID();
                    }
                }

            }

            ImGui::DrawRuler(_s_canvas);
            ImGui::EndCanvas();
        }

        return true;
    }


} // namespace fin
