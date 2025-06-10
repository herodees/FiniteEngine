#pragma once

#include "include.hpp"
#include "behavior.hpp"

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class Scene;
    struct ArchiveParams;

    namespace Sc
    {
        constexpr std::string_view Id("$id");
        constexpr std::string_view Group("$grp");
        constexpr std::string_view Name("$nme");
        constexpr std::string_view Uid("$uid");
        constexpr std::string_view Diff("$diff");
        constexpr std::string_view Class("$cls");
        constexpr std::string_view Flag("$fl");
        constexpr std::string_view Atlas("atl");
        constexpr std::string_view Sprite("spr");
    } // namespace Sc


    enum ComponentFlags_
    {
        ComponentFlags_Default           = 0,      // Default component flags
        ComponentFlags_Private           = 1 << 0, // Component is private and should not be shown in the component list
        ComponentFlags_NoWorkspaceEditor = 1 << 1, // Component should not be shown in the workspace editor
        ComponentFlags_NoEditor          = 1 << 2, // Component should not be shown in the editor at all
        ComponentFlags_NoPrefab          = 1 << 3,
    };

    using ComponentFlags = uint32_t;



    struct ComponentData
    {
        entt::registry::base_type* storage;
        std::string_view           id;
        std::string_view           label;
        ComponentFlags             flags;
        void (*emplace)(Entity);
        void (*remove)(Entity);
        bool (*contains)(Entity);
        bool (*Load)(ArchiveParams& ar);
        bool (*Save)(ArchiveParams& ar);
        bool (*ImguiProps)(Entity);
        bool (*ImguiWorkspace)(ImGui::CanvasParams&, Entity);

        inline bool HasWorkspaceEditor() const
        {
            return !(flags & ComponentFlags_NoWorkspaceEditor);
        }
        inline bool HasEditor() const
        {
            return !(flags & ComponentFlags_NoEditor);
        }
    };

    struct ComponentTag
    {
    };

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    struct Component : ComponentTag
    {
        using Storage = entt::storage_for_t<C>;

        static C& EmplaceOrReplace(Entity e)
        {
            if (!GetStorage().contains(e))
                GetStorage().emplace(e);
            return GetStorage().get(e);
        }
        static void Emplace(Entity e)
        {
            _s_storage.storage->emplace(e);
        }
        static void Remove(Entity e)
        {
            _s_storage.storage->remove(e);
        }
        static bool Contains(Entity e)
        {
            return _s_storage.storage->contains(e);
        }
        static C* Get(Entity e)
        {
            return static_cast<C*>(_s_storage.storage->get(e));
        }
        static bool Load(ArchiveParams& ar)
        {
            return true;
        }
        static bool Save(ArchiveParams& ar)
        {
            return true;
        }
        static bool ImguiProps(Entity)
        {
            return false;
        }
        static bool ImguiWorkspace(ImGui::CanvasParams&, Entity)
        {
            return false;
        }
        static Storage& GetStorage()
        {
            return static_cast<Storage&>(*_s_storage.storage);
        }
        static ComponentData& GetData(Registry& reg, ComponentFlags flags);

        static ComponentData    _s_storage;
        static std::string_view _s_id;
        static std::string_view _s_label;
    };

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline ComponentData& Component<C, ID, LABEL>::GetData(Registry& reg, ComponentFlags flags)
    {
        _s_storage.storage        = &reg.storage<C>();
        _s_storage.id             = C::_s_id;
        _s_storage.label          = C::_s_label;
        _s_storage.flags          = flags;
        _s_storage.emplace        = &C::Emplace;
        _s_storage.remove         = &C::Remove;
        _s_storage.contains       = &C::Contains;
        _s_storage.Load           = &C::Load;
        _s_storage.Save           = &C::Save;
        _s_storage.ImguiProps     = &C::ImguiProps;
        _s_storage.ImguiWorkspace = &C::ImguiWorkspace;
        return _s_storage;
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    ComponentData Component<C, ID, LABEL>::_s_storage{};

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    std::string_view Component<C, ID, LABEL>::_s_id{ID.value};

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    std::string_view Component<C, ID, LABEL>::_s_label{LABEL.value};



    struct ArchiveParams
    {
        Scene&   scene;
        Entity   entity;
        msg::Var data;

        Entity GetOldEntity(Entity oldid) const;
        bool   NameEntity(Entity eid, std::string_view name);
    };



    class ComponentFactory
    {
        friend class ImportDialog; 
    public:
        using ComponentMap = std::unordered_map<std::string, ComponentData, std::string_hash, std::equal_to<>>;
        using BehaviorMap = std::unordered_map<std::string, BehaviorData, std::string_hash, std::equal_to<>>;

        ComponentFactory(Scene& scene) : _scene(scene) {};
        ~ComponentFactory() = default;

        template <typename C>
        void RegisterComponent(ComponentFlags flags = ComponentFlags_Default)
        {
            static_assert(std::is_base_of_v<ComponentTag, C>, "Component must derive from Component");

            auto& data = C::GetData(_registry, flags);
            if (!(flags & ComponentFlags_Private))
            {
                _components.emplace(C::_s_id, C::_s_storage);
            }
        }

        template <typename C>
        void RegisterBehavior(BehaviorFlags flags = BehaviorFlags_Default)
        {
            static_assert(std::is_base_of_v<BehaviorTag, C>, "Behavior must derive from Behavior");

            auto& data = C::GetData(_registry, flags);
            if (!(flags & BehaviorFlags_Private))
            {
                auto& reg = GetRegistry();

                reg.on_construct<C>().connect([](entt::registry&, entt::entity entity) { C::Get(entity)->OnStart(); });
                reg.on_destroy<C>().connect([](entt::registry&, entt::entity entity) { C::Get(entity)->OnDestroy(); });

                _behaviors.emplace(data.id, data);
            }
        }

        Registry&     GetRegistry();
        ComponentMap& GetComponents();
        BehaviorMap&  GetBehaviors();

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

        bool ImguiComponent(Scene* scene, ComponentData* comp, Entity entity);

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
        Registry                                                                             _registry;
        std::unordered_map<std::string, Entity, std::string_hash, std::equal_to<>>           _named;
        ComponentMap                                                                         _components;
        BehaviorMap                                                                          _behaviors;
        entt::storage<Entity>                                                                _entity_map;
        std::unordered_map<uint64_t, msg::Var>                                               _prefab_map;
        msg::Var                                                                             _prefabs;
        std::unordered_map<std::string, std::vector<int>, std::string_hash, std::equal_to<>> _groups;
        std::string                                                                          _base_folder;
        std::string                                                                          _buff;
        int32_t                                                                              _selected{0};
        ComponentData*                                                                       _selected_component{};
        Entity                                                                               _edit{entt::null};
        Entity                                                                               _prefab_edit{entt::null};
        int32_t                                                                              _prefab_edit_index{-1};
        bool                                                                                 _prefab_changed{false};
        bool                                                                                 _prefab_explorer{};
    };




} // namespace fin
