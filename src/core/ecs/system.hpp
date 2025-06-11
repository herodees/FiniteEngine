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

        Register&        GetRegister();
        Scene&           GetScene();
        std::string_view GetName() const;
        SystemInfo*      GetSystemInfo() const;
        bool             ShouldRunSystem() const;
        virtual void     OnCreate() {};
        virtual void     OnDestroy() {};
        virtual void     OnStartRunning() {};
        virtual void     OnStopRunning() {};
        virtual void     Update(float dt) {};
        virtual void     FixedUpdate(float dt) {};
        virtual bool     ImguiSetup();

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
        void RegisterSystem(SystemFlags flags = SystemFlags_Default);

        void    AddDefaults();
        int32_t AddSystem(std::string_view id);
        void    DeleteSystem(int32_t sid);
        int32_t MoveSystem(int32_t sid, bool up);
        System* GetSystem(int32_t sid);
        int32_t FindSystem(std::string_view name) const;
        bool    IsValid(int32_t sid) const;
        void    OnStartRunning();
        void    OnStopRunning();
        void    Update(float dt);
        void    FixedUpdate(float dt);

        int32_t ImguiMenu();
        bool    ImguiSystems(int32_t* sys);

    private:
        Scene&                                                                         _scene;
        std::unordered_map<std::string, SystemInfo, std::string_hash, std::equal_to<>> _system_map;
        std::vector<System*>                                                           _systems;
    };


    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline void SystemManager::RegisterSystem(SystemFlags flags)
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
