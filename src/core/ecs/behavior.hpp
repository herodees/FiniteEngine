#pragma once

#include "include.hpp"

namespace fin
{
    struct IBehaviorScript;

    enum BehaviorFlags_
    {
        BehaviorFlags_Default   = 0,
        BehaviorFlags_Singleton = 1 << 0,
    };

    using BehaviorFlags = uint32_t;



    struct BehaviorInfo
    {
        std::string_view name;
        BehaviorFlags    flags{};
        IBehaviorScript* (*create)();
    };

    struct IBehaviorScript
    {
        IBehaviorScript* next{};
        BehaviorInfo*    info{};
        std::string_view type_name;

        virtual ~IBehaviorScript() {};
        virtual void OnStart(Entity e) {};
        virtual void OnUpdate(Entity e, float dt) = 0;
        virtual void OnDestroy(Entity e) {};
        virtual bool OnEdit(Entity e)
        {
            return false;
        };
        virtual void Load(msg::Var& ar) {};
        virtual void Save(msg::Var& ar) const {};
    };



    class ScriptFactory
    {
    public:
        template <class Script>
        void RegisterScript(std::string_view name);

        IBehaviorScript* Enplace(Entity ent, std::string_view name) const;
        bool             Contains(Entity ent, std::string_view name) const;
        IBehaviorScript* Get(Entity ent, std::string_view name) const;
        void             Remove(Entity ent, IBehaviorScript* script) const;

        IBehaviorScript* Create(std::string_view name) const;
    private:
        std::unordered_map<std::string, IBehaviorScript* (*)(), std::string_hash, std::equal_to<>> _registry;
    };



    template <class Script>
    inline void ScriptFactory::RegisterScript(std::string_view name)
    {
        static_assert(std::is_base_of_v<IBehaviorScript, Script>, "Script must derive from IBehaviorScript");

        _registry[std::string{name}] = []() -> IBehaviorScript* { return new Script(); };
    }

} // namespace fin
