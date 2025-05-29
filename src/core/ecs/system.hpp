#pragma once

#include "include.hpp"

namespace fin
{
    class Scene;

    enum SystemFlags_
    {
        SystemFlags_Default  = 0, // Default component flags
        SystemFlags_Private  = 1 << 0,
        SystemFlags_Disabled = 1 << 1,
    };

    using SystemFlags = uint32_t;


    class System
    {
    public:
        System(Scene& s);
        virtual ~System() = default;

        Registry& registry();
        Scene&    scene();
        int32_t   get_index() const;
        bool      should_run_system() const;

        virtual void on_create() {};
        virtual void on_destroy() {};
        virtual void on_start_runing() {};
        virtual void on_stop_runing() {};
        virtual void update(float dt) {};
        virtual void fixed_update(float dt) {};

    private:
        Scene&  _scene;
        int32_t _index{};
        SystemFlags _flags{};
    };



    class SystemFactory
    {
        struct Info
        {
            std::string_view name;
            std::string_view label;
            SystemFlags      flags;
            System* (*create)();
        };
    public:
        template <typename C, std::string_literal ID, std::string_literal LABEL>
        void register_system(SystemFlags flags = SystemFlags_Default)
        {
            auto [it, inserted] = _system_map.try_emplace(ID, Info{});
            if (inserted)
            {
                it->second.name   = ID;
                it->second.label  = LABEL;
                it->second.flags  = flags;
                it->second.create = [flags]() -> System*
                {
                    auto* sys   = new C();
                    sys->_flags = flags;
                    return sys;
                };
            }
        }
        std::unordered_map<std::string, Info> _system_map;
    };
}
