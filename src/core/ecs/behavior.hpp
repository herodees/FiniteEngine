#pragma once

#include "include.hpp"

namespace fin
{
    struct IBehaviorScript;
    class Scene;

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
        Scene& scene;
    };



    struct IBehaviorScript
    {
        IBehaviorScript*    next{};
        const BehaviorInfo* info{};

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
        ScriptFactory(Scene& scene);
        ~ScriptFactory();

        template <class Script>
        void RegisterScript(std::string_view name, BehaviorFlags flags);

        IBehaviorScript* Enplace(Entity ent, std::string_view name) const;
        bool             Contains(Entity ent, std::string_view name) const;
        IBehaviorScript* Get(Entity ent, std::string_view name) const;
        void             Remove(Entity ent, IBehaviorScript* script) const;

        IBehaviorScript* Create(std::string_view name) const;

    private:
        Scene&                                                                           _scene;
        std::unordered_map<std::string, BehaviorInfo, std::string_hash, std::equal_to<>> _registry;
    };



    template <class Script>
    inline void ScriptFactory::RegisterScript(std::string_view name, BehaviorFlags flags)
    {
        static_assert(std::is_base_of_v<IBehaviorScript, Script>, "Script must derive from IBehaviorScript");
        auto [it, inserted] = _registry.emplace(std::string(name), {"", flags, nullptr, _scene});
        if (inserted)
        {
            it->second.name   = it->first;
            it->second.create = []() -> IBehaviorScript* { return new Script(); };
        }
    }

} // namespace fin
