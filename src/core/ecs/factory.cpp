#include "factory.hpp"
#include "builtin.hpp"
#include <core/scene.hpp>
#include <core/editor/imgui_control.hpp>
#include <utils/imguiline.hpp>
#include <utils/dialog_utils.hpp>
#include <core/application.hpp>

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

    static ImGui::CanvasParams _s_canvas;
    static msg::Var            _s_copy;
    static ComponentInfo*      _s_comp{};

    void          ResetAtlasCache();
    Sprite2D::Ptr LoadSpriteChache(std::string_view spr);

    class ImportDialog : public ImGui::Dialog
    {
    public:
        ImportDialog(ComponentFactory& factory, const std::string& dir) : _factory(factory)
        {
            _components.resize(_factory.GetRegister().GetComponents().size());

            auto files = ComponentFactory::GetFolderFiles(dir, ".sprite");
            for (auto& s : files)
            {
                _factory.NormalizeAssetPath(s);
                _files.emplace_back(std::move(s), true);
            }
            _valid = !_files.empty();
        }

        bool OnImport()
        {
            for (auto i = 0; i < _files.size(); ++i)
            {
                auto& spr = _files[i];
                if (!spr.second)
                    continue;
                auto  obj = create_empty_prefab(GetFileNameWithoutExt(spr.first.c_str()), _group);
                auto  cls = obj.get_item(Sc::Class);

                auto&  cmps = _factory.GetRegister().GetComponents();
                size_t n    = 0;
                for (auto& el : cmps)
                {
                    if (el->id == CSprite2D::CID)
                    {
                        msg::Var ar;
                        ar.set_item("src", spr.first);
                        cls.set_item(el->id, ar);
                    }
                    else if (_components[n])
                    {
                        cls.set_item(el->id, nullptr);
                    }
                    ++n;
                }
                _factory.PushPrefabData(obj);
            }
            _factory.GeneratePrefabMap();
            return true;
        }

        bool OnUpdate() override
        {
            if (ImGui::InputText("Group", &_group))
            {
            }
            if (ImGui::BeginCombo("Components", "Select Component", 0))
            {
                auto&  cmps = _factory.GetRegister().GetComponents();
                size_t n    = 0;
                for (auto& el : cmps)
                {
                    if (el->owner)
                    {
                        ++n;
                        continue;
                    }
                    if (el->id == CSprite2D::CID || el->id == CBase::CID)
                    {
                        _components[n] = true;
                    }
                    ImGui::Checkbox(el->label.data(), &(bool&)_components[n++]);
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginChildFrame(ImGui::GetID(this), {-1, -1 * ImGui::GetFrameHeightWithSpacing()}) && !_files.empty())
            {
                for (auto& file : _files)
                {
                    ImGui::Checkbox(file.first.c_str(), &file.second);
                }
            }
            ImGui::EndChildFrame();

            if (ImGui::LineItem("ftr", {-1, ImGui::GetFrameHeight()})
                    .Spring()
                    .PushStyle(ImStyle_Button, 1)
                    .Text("  Cancel  ")
                    .PopStyle()
                    .Space()
                    .PushStyle(ImStyle_Header, 2)
                    .Text("   Import   ")
                    .PopStyle()
                    .End())
            {
                if (ImGui::Line().HoverId() == 1)
                    _valid = false;
                if (ImGui::Line().HoverId() == 2 && OnImport())
                    _valid = false;
            }
            return _valid;
        }

        ComponentFactory&    _factory;
        std::string          _path;
        std::string          _group;
        std::vector<uint8_t> _components;
        std::vector<std::pair<std::string, bool>> _files;
        bool                 _valid = true;
    };



    ComponentFactory::~ComponentFactory()
    {
        for (auto& el : _registry.GetComponents())
        {
            if (el->owner == nullptr)
            {
                delete el;
            }
        }
    }

    Register& ComponentFactory::GetRegister()
    {
        return _registry;
    }

    Entity ComponentFactory::GetOldEntity(Entity old_id)
    {
        if (old_id == entt::null)
            return entt::null;

        if (!_entity_map.contains(old_id))
        {
            auto e = _registry.Create();
            _entity_map.emplace(old_id, e);
            return e;
        }

        return _entity_map.get(old_id);
    }

    void ComponentFactory::ClearOldEntities()
    {
        _entity_map.clear();
    }


    void ComponentFactory::LoadEntity(Entity& entity, msg::Var& data)
    {
        auto old_entity = (Entity)data[Sc::Id].get((uint32_t)entt::null); // Ensure Id is present
        entity          = GetOldEntity(old_entity);
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

            if (!Contains<CPrefab>(entity))
            {
                Emplace<CPrefab>(entity); // Ensure the Prefab component exists for the entity
            }
            auto& cmp = Get<CPrefab>(entity);
            cmp._data = it->second;                             // Store the prefab data in the component
            auto Load = it->second.get_item(Sc::Class).clone(); // Clone the prefab data
            Load.apply(cls);                                    // Apply the class data to the cloned prefab data
            LoadPrefabComponent(entity, Load);               // Load the components from the class data
        }
        else
        {
            LoadPrefabComponent(entity, cls); // Load the components from the class data
        }
    }

    void ComponentFactory::SaveEntity(Entity entity, msg::Var& data)
    {
        data.set_item(Sc::Id, (uint32_t)entity);
        msg::Var cls;
        if (auto* base = Find<CPrefab>(entity))
        {
            SavePrefabComponent(entity, cls);

            msg::Var diff = base->_data.get_item(Sc::Class).diff(cls);
            data.set_item(Sc::Uid, base->_data.get_item(Sc::Uid));
            data.set_item(Sc::Class, diff);
        }
        else
        {
            SavePrefabComponent(entity, cls);
            data.set_item(Sc::Class, cls);
        }
    }

    void ComponentFactory::OnLayerUpdate(float dt, SparseSet& active)
    {
        for (auto* comp : _registry.GetComponents())
        {
            if (!(comp->flags & ComponentsFlags_Script))
                continue;

            SparseSet* smallset = &active;
            SparseSet* bigset   = comp->storage;
            if (bigset->size() < smallset->size())
                std::swap(smallset, bigset);

            for (auto ent : *smallset)
            {
                if (bigset->contains(ent))
                {
                    static_cast<IScriptComponent*>(comp->storage->get(ent))->Update(dt);
                }
            }
        }
    }

    bool ComponentFactory::IsPrefabMode() const
    {
        return _prefab_explorer;
    }

    bool ComponentFactory::SetEntityName(Entity entity, std::string_view name)
    {
        if (name.empty())
        {
            if (auto* nme = Find<CName>(entity))
            {
                _named.erase(std::string(nme->_name));
                Erase<CName>(entity);
                return true;
            }
            return false;
        }

        auto [it, inserted] = _named.try_emplace(std::string(name), entity);
        if (inserted)
        {
            if (auto* base = Find<CName>(entity))
            {
                _named.erase(std::string(base->_name));
                base->_name = it->first;
            }
            else
            {
                Emplace<CName>(entity);
                auto& nme  = Get<CName>(entity);
                nme._name = it->first;
            }
            return true;
        }
        return false;
    }

    std::string_view ComponentFactory::GetEntityName(Entity entity) const
    {
        if (auto* nme = Find<CName>(entity))
        {
            return nme->_name;
        }
        return std::string_view();
    }

    Entity ComponentFactory::GetEntityByName(std::string_view entity) const
    {
        auto it = _named.find(entity);
        return it == _named.end() ? entt::null : it->second;
    }



    int ComponentFactory::PushPrefabData(msg::Var& obj)
    {
        int n = _prefabs.size();
        _prefabs.push_back(obj);
        _prefab_map[obj.get_item(Sc::Uid).get(0ull)] = obj;
        _groups[""].push_back(n);
        return n;
    }

    void ComponentFactory::LoadPrefabComponent(Entity entity, msg::Var& data)
    {
        for (auto e : data.members())
        {
            auto c = e.first.str();
            if (c[0] == '$')
                continue;

            auto* nfo = _registry.GetComponentInfoById(c);
            if (!nfo)
            {
                TraceLog(LOG_WARNING, "Component %.*s not registered", c.size(), c.data());
                continue;
            }

            if (!nfo->Contains(entity))
            {
                nfo->Emplace(entity);
            }

            ArchiveParams ap{entity, e.second};
            if (!nfo->OnDeserialize(ap))
            {
                TraceLog(LOG_WARNING, "Component %.*s not loaded", c.size(), c.data());
            }
        }
    }

    void ComponentFactory::SavePrefabComponent(Entity entity, msg::Var& data)
    {
        auto& components = _registry.GetComponents();
        data.make_object(components.size());
        data.erase();
        for (auto* cmp : components)
        {
            if (cmp->Contains(entity))
            {
                if (cmp->flags & ComponentsFlags_Private)
                    continue;
                ArchiveParams ap{entity};
                cmp->OnSerialize(ap);
                if (cmp->flags & ComponentsFlags_NoEmpty && ap.data.is_undefined())
                    continue;
                data.set_item(cmp->id, ap.data);
            }
        }
    }

    void ComponentFactory::DuplicatePrefab(Scene* scene, int32_t n)
    {
        if ((uint32_t)n >= _prefabs.size())
            return;

        auto el = _prefabs.get_item(n);
        auto newel = el.clone();
        newel.set_item(Sc::Uid, std::generate_unique_id());
        auto nme = newel.get_item(Sc::Id);
        newel.set_item(Sc::Id, std::string(nme.str()) + "_copy");

        _prefabs.push_back(newel);
        _prefab_map[newel.get_item(Sc::Uid).get(0ull)] = newel;

        SelectPrefab(scene, _prefabs.size() - 1);
        GeneratePrefabMap();
    }

    void ComponentFactory::SelectPrefab(Scene* scene, int32_t n)
    {
        if (_edit != entt::null)
        {
            _registry.Destroy(_edit);
            _edit = entt::null;
        }
        _selected = n;
        if ((uint32_t)_selected >= _prefabs.size())
            return;

        _edit = _registry.Create();
        auto el = _prefabs.get_item(n);
        auto cls = el.get_item(Sc::Class);
        LoadPrefabComponent(_edit, cls);

        if (!Contains<CPrefab>(_edit))
        {
            Emplace<CPrefab>(_edit); // Ensure the Prefab component exists for the entity
        }
        Get<CPrefab>(_edit)._data = el;
    }

    void ComponentFactory::EditPrefab(Scene* scene, int32_t n)
    {
        if (_prefab_edit != entt::null)
        {
            if (_prefab_changed && 1 ==
                messagebox_yes_no("Edit Prefab", "Do you want to save the current prefab before editing?"))
            {
                SaveEditPrefab(scene);
            }
            CloseEditPrefab(scene);
        }

        _prefab_edit_index = n;
        auto el            = _prefabs.get_item(n);
        auto cls           = el.get_item(Sc::Class);
        _prefab_edit       = _registry.Create();
        LoadPrefabComponent(_prefab_edit, cls);
        _prefab_changed = false;
    }

    void ComponentFactory::SaveEditPrefab(Scene* scene)
    {
        if (_prefab_edit == entt::null)
            return;

        auto prefab = _prefabs.get_item(_prefab_edit_index);
        auto cls = prefab.get_item(Sc::Class);
        SavePrefabComponent(_prefab_edit, cls);

    }

    void ComponentFactory::CloseEditPrefab(Scene* scene)
    {
        if (_prefab_edit == entt::null)
            return;
        scene->GetFactory().GetRegister().Destroy(_prefab_edit);
        _prefab_edit = entt::null;
        _prefab_edit_index = -1;
    }

    void ComponentFactory::GeneratePrefabMap()
    {
        _groups.clear();
        for (int i = 0; i < _prefabs.size(); i++)
        {
            auto val   = _prefabs.get_item(i);
            auto group = val.get_item(Sc::Group); // assuming std::string
            _groups[group.get("")].push_back(i);
        }
    }


    bool ComponentFactory::Load()
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

        return Load(doc);
    }

    bool ComponentFactory::Load(msg::Var& ar)
    {
        auto itm = ar.get_item("items");
        _prefabs = itm;
        _prefab_map.clear();
        for (auto el : _prefabs.elements())
        {
            auto key = el.get_item(Sc::Uid).get(0ull);
            _prefab_map[key] = el;
        }
        GeneratePrefabMap();
        return true;
    }

    bool ComponentFactory::Save(msg::Var& ar)
    {
        ar.set_item("items", _prefabs);
        return true;
    }

    bool ComponentFactory::Save()
    {
        msg::Var doc;
        if (!Save(doc))
            return false;

        std::string str;
        doc.to_string(str);
        return SaveFileText((_base_folder + GamePrefabFile).c_str(), str.c_str());
    }

    bool ComponentFactory::ImguiMenu(Scene* scene)
    {
        auto& components = _registry.GetComponents();
        ImGui::LineItem(ImGui::GetID("prefab"), {-1, ImGui::GetFrameHeight()}).Space();

        int32_t n = 1;
        for (auto* cmp : components)
        {
            if (!(cmp->flags & ComponentsFlags_NoWorkspaceEditor) && cmp->Contains(_prefab_edit))
            {
                ImGui::Line()
                    .PushStyle(ImStyle_Header, n, _selected_component == cmp)
                    .Space()
                    .Text(cmp->label.data())
                    .Space()
                    .PopStyle();
            }
            ++n;
        }

        ImGui::Line()
            .Spring()

            .PushStyle(ImStyle_Button, -99)
            .Tooltip("Open grid snap settings")
            .Text(ICON_FA_BORDER_NONE)
            .Space()
            .Text(gSettings.grid_snap ? ImGui::FormatStr("%dx%d", gSettings.grid_snapx, gSettings.grid_snapy) : "Off")
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Header, -5)
            .Tooltip("Center the origin of the canvas")
            .Space()
            .Text(ICON_FA_ARROWS_TO_CIRCLE)
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Header, -4)
            .Tooltip("Reset zoom to 100%")
            .Format("%.1f%%", _s_canvas.zoom * 100)
            .PopStyle()
            .Space(16)
            .PushStyle(ImStyle_Header, -3)
            .Tooltip("Save the current prefab")
            .Space()
            .Text(ICON_FA_FLOPPY_DISK " Save")
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Header, -2)
            .Tooltip("Reset the current prefab to the original state")
            .Space()
            .Text("Close")
            .Space()
            .PopStyle()
            .Space();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == -5)
            {
                _s_canvas.CenterOnScreen();
            }
            else if(ImGui::Line().HoverId() == -4)
            {
                _s_canvas.zoom = 1.0f;
            }
            else if (ImGui::Line().HoverId() == -99)
            {
                ImGui::OpenPopup("GridSnapMenu");
            }
            else if(ImGui::Line().HoverId() < 0)
            {
                if (ImGui::Line().HoverId() == -3)
                {
                    SaveEditPrefab(scene);
                }
                scene->SetMode(SceneMode::Scene);
                CloseEditPrefab(scene);
            }
            else
            {
                auto it = components.begin();
                std::advance(it, ImGui::Line().HoverId() - 1);
                _selected_component = *it;
            }
        }

        if (ImGui::BeginPopup("GridSnapMenu"))
        {
            ImGui::DragInt2("Snap", &gSettings.grid_snapx, 1.0f, 1.0f, 100.0f);
            if (ImGui::Checkbox("Snap to Grid", &gSettings.grid_snap))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        return true;
    }

    bool ComponentFactory::ImguiComponent(Scene* scene, ComponentInfo* comp, Entity entity)
    {
        bool ret = false;
        const auto script = comp->flags & ComponentsFlags_Script;
        const auto clr    = script ? 0xffa0a0ff : ImGui::GetColorU32(ImGuiCol_ButtonActive);

        ImGui::LineItem(comp->id.data(), {-1, ImGui::GetFrameHeightWithSpacing()}).Expandable(true).Space();

        ImGui::PushID(comp->id.data());
        ImGui::Line()
            .PushColor(clr)
            .Text(ImGui::Line().Expanded() ? ICON_FA_CARET_DOWN " " : ICON_FA_CARET_RIGHT " ")
            .Text(comp->label.data())
            .PopColor()
            .Spring();

        if (!(comp->flags & ComponentsFlags_NoWorkspaceEditor))
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
                ArchiveParams ar{entity};
                comp->OnDeserialize(ar);
            }
            ImGui::Separator(); 
            if (ImGui::MenuItem("Remove Component", 0, false, comp->id != "_"))
            {
                comp->Erase(entity);
                ret = true;
            }
            ImGui::Separator(); 
            if (ImGui::MenuItem("Copy Component"))
            {
                ArchiveParams ar{entity};
                comp->OnSerialize(ar);
                _s_copy = ar.data; // Store the component data in a global variable for pasting later
                _s_comp = comp;    // Store the component pointer for pasting later
            }
            if (ImGui::MenuItem("Paste Component", 0, false, _s_comp && _s_copy.is_object()) && _s_comp)
            {
                if (!_s_comp->Contains(entity)) // Ensure the component exists for the entity
                    _s_comp->Emplace(entity);   // If not, create it
                ArchiveParams ar{entity, _s_copy};
                _s_comp->OnDeserialize(ar);
            }
            ImGui::EndPopup();
        }

        if (ImGui::Line().Expanded() && !ret && comp)
            ret = comp->OnEdit(entity);

        ImGui::PopID();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));
        ImGui::Separator();
        ImGui::PopStyleColor();
        return ret;
    }

    bool ComponentFactory::ImguiPrefab(Scene* scene, Entity ImguiProps)
    {
        bool ret = false;
        int add = 0;

        auto& components = _registry.GetComponents();
        for (auto* component : components)
        {
            if (component->flags & ComponentsFlags_Private)
                continue;
            if (component->Contains(ImguiProps))
            {
                ret |= ImguiComponent(scene, component, ImguiProps);
            }
        }

        ImGui::LineItem(ImGui::GetID("crbtn"), {-1, ImGui::GetFrameHeight()})
            .Spring()
            .PushStyle(ImStyle_Button, 1)
            .Space()
            .Text(ICON_FA_PLUS " Add Component")
            .Space()
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Header, 2)
            .Space()
            .Text(ICON_FA_PLUS " Add Script")
            .Space()
            .PopStyle()
            .Spring()
            .End();

        if (ImGui::Line().Return())
        {
            if (ImGui::Line().HoverId() == 1)
                add = 1;
            else if (ImGui::Line().HoverId() == 2)
                add = 2;
        }

        if (ImGui::BeginPopupContextWindow("BackgroundPopup", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_FA_PLUS " Add Component"))
                add = 1;
            if (ImGui::MenuItem(ICON_FA_PLUS " Add Script"))
                add = 2;
            if (ImGui::MenuItem("Paste Component", 0, false, _s_comp && _s_copy.is_object()))
            {
                if (!_s_comp->Contains(ImguiProps)) // Ensure the component exists for the entity
                    _s_comp->Emplace(ImguiProps);   // If not, create it
                ArchiveParams ar{ImguiProps, _s_copy};
                _s_comp->OnDeserialize(ar);
            }
            ImGui::EndPopup();
        }

        if (add == 1)
            ImGui::OpenPopup("ComponentMenu");
        if (ImGui::BeginPopup("ComponentMenu"))
        {
            for (auto* comp : components)
            {
                if (comp->flags & ComponentsFlags_Private || comp->flags & ComponentsFlags_Script)
                    continue;

                const char* plugin = comp->owner ? comp->owner->GetInfo().name.data() : nullptr;
                if (ImGui::MenuItem(comp->label.data(), plugin, false, !comp->Contains(ImguiProps)))
                {
                    comp->Emplace(ImguiProps);
                    ret = true;
                }
            }
            ImGui::EndPopup();
        }

        if (add == 2)
            ImGui::OpenPopup("ScriptMenu");
        if (ImGui::BeginPopup("ScriptMenu"))
        {
            for (auto* comp : components)
            {
                if (comp->flags & ComponentsFlags_Private || !(comp->flags & ComponentsFlags_Script))
                    continue;

                const char* plugin = comp->owner ? comp->owner->GetInfo().name.data() : nullptr;
                if (ImGui::MenuItem(comp->label.data(), plugin, false, !comp->Contains(ImguiProps)))
                {
                    comp->Emplace(ImguiProps);
                    ret = true;
                }
            }
            ImGui::EndPopup();
        }
        return ret;
    }

    void ComponentFactory::ImguiProperties(Scene* scene)
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
                    GeneratePrefabMap();
                }
            };
    

        if (_prefab_edit != entt::null)
        {
            show_prefab_names(_prefab_edit_index);
            if (ImguiPrefab(scene, _prefab_edit))
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

        if (auto obj = Find<CBase>(_edit))
        {
            if (obj->_layer)
            {
                _edit = entt::null;
            }
        }

        show_prefab_names(_selected);
        ImGui::BeginDisabled(true);
        ImguiPrefab(scene, _edit);
        ImGui::EndDisabled();
    }

    void ComponentFactory::ImguiPrefabs(Scene* scene)
    {
        int context_id = -1;

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
                        SelectPrefab(scene, n);
                    }

                    if (ImGui::IsMouseClicked(1) && ImGui::IsItemHovered())
                    {
                        context_id = n;
                    }

                    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
                    {
                        scene->SetMode(SceneMode::Prefab);
                        EditPrefab(scene, n);
                    }

                    if (ImGui::IsItemVisible())
                    {
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                        {
                            if (_edit == entt::null || _selected != n)
                            {
                                SelectPrefab(scene, n);
                            }

                            ImGui::SetDragDropPayload("ENTITY", &_edit, sizeof(Entity));
                            ImGui::EndDragDropSource();
                        }

                        auto spr = val.get_item(Sc::Class).get_item(CSprite2D::CID);
                        if (spr.is_undefined())
                        {
                            ImGui::SameLine();
                            ImGui::SpriteImage((Sprite2D*)nullptr, {25, 25});
                        }
                        else if (auto pack = LoadSpriteChache(spr.get_item("src").str()))
                        {
                            ImGui::SameLine();
                            ImGui::SpriteImage(pack.get(), {25, 25});
                        }
                        else
                        {
                            ImGui::SameLine();
                            ImGui::SpriteImage((Sprite2D*)nullptr, {25, 25});
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

        if (context_id >= 0)
        {
            ImGui::OpenPopup("PrefabContextMenu");
            SelectPrefab(scene, context_id);
        }

        if(ImGui::BeginPopupContextItem("PrefabContextMenu"))
        {
            if (ImGui::MenuItem("Edit Prefab"))
            {
                scene->SetMode(SceneMode::Prefab);
                EditPrefab(scene, _selected);
            }
            if (ImGui::MenuItem("Duplicate Prefab"))
            {
                DuplicatePrefab(scene, _selected);
            }

            ImGui::EndPopup();
        }
    }

    void ComponentFactory::ImguiItems(Scene* scene)
    {
        ImGui::LineItem("astmnu", {-1, ImGui::GetFrameHeight()})
            .Space()
            .PushStyle(ImStyle_Button, 1, !_prefab_explorer)
            .Tooltip("Show File Explorer")
            .Space()
            .Text(ICON_FA_FOLDER_TREE " Assets")
            .Space()
            .PopStyle()
            .PushStyle(ImStyle_Button, 2, _prefab_explorer)
            .Tooltip("Show Prefab Explorer")
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
                .Tooltip("Create New Prefab")
                .Text(ICON_FA_FILE_CIRCLE_PLUS)
                .PopStyle()
                .PushStyle(ImStyle_Button, 11)
                .Tooltip("Save All Prefabs")
                .Text(ICON_FA_FLOPPY_DISK)
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 20)
                .Tooltip("Delete Prefab")
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
                    int n = PushPrefabData(obj);
                    SelectPrefab(scene, n);
                }
                break;
                case 11:
                    SelectPrefab(scene, _selected);
                    Save();
                    break;
                case 20:
                {
                    if ((uint32_t)_selected < _prefabs.size())
                    {
                        if (_prefab_edit == entt::null)
                        {
                            auto t = _selected;
                            SelectPrefab(scene, -1);
                            _prefabs.erase(t);
                            SelectPrefab(scene, t);
                            GeneratePrefabMap();
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
                ImguiPrefabs(scene);
            }
            else
            {
                ImguiExplorer(scene);
            }
        }
        ImGui::EndChildFrame();
    }

    void ComponentFactory::ImguiShowPrefabImport()
    {
        auto dir = open_folder_dialog("Import Atlas Folder", _base_folder);
        if (!dir.empty() && IsAssetFolder(dir))
        {
            ImGui::Dialog::Show<ImportDialog>("Import Sprites", {800, 600}, 0, *this, dir);
        }
    }

    bool ComponentFactory::ImguiWorkspace(Scene* scene)
    {
        static bool init_canvas = true;

        if (gSettings.grid_snap)
        {
            _s_canvas.snap_grid.x = float(gSettings.grid_snapx);
            _s_canvas.snap_grid.y = float(gSettings.grid_snapy);
        }
        else
        {
            _s_canvas.snap_grid = {};
        }

        if (ImGui::BeginCanvas("SceneCanvas", ImVec2(0, 0), _s_canvas))
        {
            if (init_canvas)
            {
                _s_canvas.CenterOnScreen();
                init_canvas = false;
            }

            ImGui::DrawGrid(_s_canvas);
            ImGui::DrawOrigin(_s_canvas, -1);

            if (_prefab_edit != entt::null && Contains<CBase>(_prefab_edit))
            {
                if (auto* s = Find<CSprite2D>(_prefab_edit))
                {
                    if (s->_spr && s->_spr->GetTexture())
                    {
                        Regionf reg(s->GetRegion({}));
                        Regionf uvs(s->_spr->GetUVRegion());

                        ImGui::DrawTexture(_s_canvas,
                                           (ImTextureID)s->_spr->GetTexture()->get_texture(),
                                           {reg.x1, reg.y1},
                                           {reg.x2, reg.y2},
                                           {uvs.x1, uvs.y1},
                                           {uvs.x2, uvs.y2},
                                           0xffffffff);
                    }
                }

                if (_selected_component && !(_selected_component->flags & ComponentsFlags_NoWorkspaceEditor))
                {
                    if (_selected_component->Contains(_prefab_edit))
                    {
                        ImGui::PushID(_selected_component);
                        _selected_component->OnEditCanvas(_prefab_edit, _s_canvas);
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
