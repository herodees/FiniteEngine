#pragma once

#include "include.hpp"
#include <api/components.hpp>
#include <api/register.hpp>

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class Scene;
    class ComponentFactory
    {
        friend class ImportDialog; 
    public:

        ComponentFactory(Scene& scene) : _scene(scene) {};
        ~ComponentFactory();

        Register&     GetRegister();

        Entity GetOldEntity(Entity old_id);
        void   ClearOldEntities();

        bool ImguiMenu(Scene* scene);
        void ImguiProperties(Scene* scene);
        void ImguiItems(Scene* scene);
        bool ImguiWorkspace(Scene* scene);
        bool ImguiPrefab(Scene* scene, Entity entity);

        void SetRoot(const std::string& startPath);

        bool Load();
        bool Save();

        void LoadEntity(Entity& entity, msg::Var& ar);
        void SaveEntity(Entity entity, msg::Var& ar);

        bool             SetEntityName(Entity entity, std::string_view name);
        std::string_view GetEntityName(Entity entity) const;
        Entity           GetEntityByName(std::string_view entity) const;

    private:
        int  PushPrefabData(msg::Var& obj);
        void GeneratePrefabMap();

        bool ImguiComponent(Scene* scene, ComponentInfo* comp, Entity entity);

        void LoadPrefabComponent(Entity entity, msg::Var& ar);
        void SavePrefabComponent(Entity entity, msg::Var& ar);

        bool Load(msg::Var& ar);
        bool Save(msg::Var& ar);

        void ImguiPrefabs(Scene* scene);
        void ImguiExplorer(Scene* scene);

        void SelectPrefab(Scene* scene, int32_t n);
        void EditPrefab(Scene* scene, int32_t n);
        void SaveEditPrefab(Scene* scene);
        void CloseEditPrefab(Scene* scene);

    private:
        Scene&                                                                               _scene;
        Register                                                                             _registry;
        std::unordered_map<std::string, Entity, std::string_hash, std::equal_to<>>           _named;
        entt::storage<Entity>                                                                _entity_map;
        std::unordered_map<uint64_t, msg::Var>                                               _prefab_map;
        msg::Var                                                                             _prefabs;
        std::unordered_map<std::string, std::vector<int>, std::string_hash, std::equal_to<>> _groups;
        std::string                                                                          _base_folder;
        std::string                                                                          _buff;
        int32_t                                                                              _selected{0};
        ComponentInfo*                                                                       _selected_component{};
        Entity                                                                               _edit{entt::null};
        Entity                                                                               _prefab_edit{entt::null};
        int32_t                                                                              _prefab_edit_index{-1};
        bool                                                                                 _prefab_changed{false};
        bool                                                                                 _prefab_explorer{};
    };




} // namespace fin
