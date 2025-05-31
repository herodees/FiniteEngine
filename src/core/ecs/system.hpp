#pragma once

#include "include.hpp"

namespace fin
{
    class Scene;
    struct SystemInfo;

    enum SystemFlags_
    {
        SystemFlags_Default  = 0, // Default component flags
        SystemFlags_Extra    = 1 << 0,
        SystemFlags_Disabled = 1 << 1,
    };

    using SystemFlags = uint32_t;

    class System
    {
        friend class SystemManager;

    public:
        System(Scene& s);
        virtual ~System() = default;

        Registry&        registry();
        Scene&           scene();
        std::string_view name() const;
        SystemInfo*      info() const;
        bool             should_run_system() const;
        virtual void     on_create() {};
        virtual void     on_destroy() {};
        virtual void     on_start_runing() {};
        virtual void     on_stop_runing() {};
        virtual void     update(float dt) {};
        virtual void     fixed_update(float dt) {};
        virtual bool     imgui_setup();

    private:
        Scene&      _scene;
        int32_t     _index{};
        SystemFlags _flags{};
        SystemInfo* _info{nullptr};
    };



    struct SystemInfo
    {
        std::string_view name;
        std::string_view label;
        SystemFlags      flags;
        System* (*create)(Scene& s, SystemInfo* i);
    };

    class SystemManager
    {
    public:
        SystemManager(Scene& scene);
        ~SystemManager();

        template <typename C, std::string_literal ID, std::string_literal LABEL>
        void register_system(SystemFlags flags = SystemFlags_Default);

        void    add_defaults();
        int32_t add_system(std::string_view id);
        void    delete_system(int32_t sid);
        int32_t move_system(int32_t sid, bool up);
        System* get_system(int32_t sid);
        int32_t find_system(std::string_view name) const;
        bool    is_valid(int32_t sid) const;
        void    on_start_runing();
        void    on_stop_runing();
        void    update(float dt);
        void    fixed_update(float dt);

        int32_t imgui_menu();
        bool    imgui_systems(int32_t* sys);
    private:
        Scene&                                                                         _scene;
        std::unordered_map<std::string, SystemInfo, std::string_hash, std::equal_to<>> _system_map;
        std::vector<System*>                                                           _systems;
    };



    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline void SystemManager::register_system(SystemFlags flags)
    {
        static_assert(std::is_base_of_v<System, C>, "System must derive from ecs::System");

        std::string id(ID.value);
        auto [it, inserted] = _system_map.emplace(id, SystemInfo());
        if (inserted)
        {
            it->second.name   = ID.value;
            it->second.label  = LABEL.value;
            it->second.flags  = flags;
            it->second.create = [](Scene& s, SystemInfo* i) -> System*
            {
                auto* sys   = new C(s);
                sys->_flags = i->flags;
                sys->_info  = i;
                return sys;
            };
        }
    }
} // namespace fin
