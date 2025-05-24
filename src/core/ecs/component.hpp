#pragma once

#include "include.hpp"

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class Scene;

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

    struct ArchiveParams
    {
        Registry&        reg;
        Entity           entity;
        msg::Var         data;
    };

    struct ComponentData
    {
        entt::registry::base_type* storage;
        std::string_view           id;
        std::string_view           label;
        void (*emplace)(Entity);
        void (*remove)(Entity);
        bool (*contains)(Entity);
        bool (*load)(ArchiveParams& ar);
        bool (*save)(ArchiveParams& ar);
        bool (*edit)(Entity);
        bool (*edit_canvas)(ImGui::CanvasParams&, Entity);
    };

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    struct Component
    {
        using Storage = entt::storage_for_t<C>;

        static C& emplace_or_replace(Entity e)
        {
            if (!storage().contains(e))
                storage().emplace(e);
            return storage().get(e);
        }
        static void emplace(Entity e)
        {
            _s_storage.storage->emplace(e);
        }
        static void remove(Entity e)
        {
            _s_storage.storage->remove(e);
        }
        static bool contains(Entity e)
        {
            return _s_storage.storage->contains(e);
        }
        static C* get(Entity e)
        {
            return static_cast<C*>(_s_storage.storage->get(e));
        }
        static bool load(ArchiveParams& ar)
        {
            return true;
        }
        static bool save(ArchiveParams& ar)
        {
            return true;
        }
        static bool edit(Entity)
        {
            return false;
        }
        static bool edit_canvas(ImGui::CanvasParams&, Entity)
        {
            return false;
        }
        static Storage& storage()
        {
            return static_cast<Storage&>(*_s_storage.storage);
        }
        static ComponentData    _s_storage;
        static std::string_view _s_id;
        static std::string_view _s_label;
    };

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    ComponentData Component<C, ID, LABEL>::_s_storage{};

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    std::string_view Component<C, ID, LABEL>::_s_id{ID.value};

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    std::string_view Component<C, ID, LABEL>::_s_label{LABEL.value};

    class ComponentFactory
    {
    public:
        using Map = std::unordered_map<std::string, ComponentData, std::string_hash, std::equal_to<>>;

        ComponentFactory()  = default;
        ~ComponentFactory() = default;

        template <typename C>
        void register_component()
        {
            C::_s_storage.storage = &_registry.storage<C>();
            C::_s_storage.id       = C::_s_id;
            C::_s_storage.label    = C::_s_label;
            C::_s_storage.emplace  = &C::emplace;
            C::_s_storage.remove   = &C::remove;
            C::_s_storage.contains = &C::contains;
            C::_s_storage.load     = &C::load;
            C::_s_storage.save     = &C::save;
            C::_s_storage.edit     = &C::edit;
            C::_s_storage.edit_canvas = &C::edit_canvas;
            _components.insert(std::pair(C::_s_id, C::_s_storage));
        }

        Registry& get_registry();
        Map& get_components();

        Entity get_old_entity(Entity old_id);
        void clear_old_entities();

        bool imgui_menu(Scene* scene);
        void imgui_props(Scene* scene);
        void imgui_items(Scene* scene);
        bool imgui_workspace(Scene* scene);
        bool imgui_prefab(Scene* scene, Entity e);

        void set_root(const std::string& startPath);

        bool load();
        bool save();

        void load_prefab(Entity e, msg::Var& prefab, msg::Var& diff);
        void save_prefab(Entity e, msg::Var& prefab, msg::Var& diff);

        void load_prefab(Entity e, msg::Var& ar);
        void save_prefab(Entity e, msg::Var& ar);

    private:
        bool load(msg::Var& ar);
        bool save(msg::Var& ar);
        void serialize(msg::Var& ar);
        void deserialize(msg::Var& ar);
        void imgui_prefabs(Scene* scene);
        void imgui_explorer(Scene* scene);
        void selet_prefab(int32_t n);
        void edit_prefab(int32_t n);
        void save_edit_prefab(Scene* scene);
        void close_edit_prefab(Scene* scene);
        void generate_prefab_map();

        msg::Var create_empty_prefab(std::string_view name, std::string_view group = "");

    private:
        Registry                                                                             _registry;
        Map                                                                                  _components;
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
        bool                                                                                 _prefab_explorer{};
    };




} // namespace fin
