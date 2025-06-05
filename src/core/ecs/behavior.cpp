#include "behavior.hpp"
#include "base.hpp"

namespace fin
{
    ScriptFactory::ScriptFactory(Scene& scene) : _scene(scene)
    {
    }

    ScriptFactory::~ScriptFactory()
    {
    }

    IBehaviorScript* ScriptFactory::Enplace(Entity ent, std::string_view name) const
    {
        if (!ecs::Script::Contains(ent))
            ecs::Script::Emplace(ent);

        auto* s = Create(name);
        if (s == nullptr)
            return nullptr;

        auto* scr = ecs::Script::Get(ent);
        scr->AddScript(s);
        s->entity = ent;
        s->OnStart();
        return s;
    }

    bool ScriptFactory::Contains(Entity ent, std::string_view name) const
    {
        return Get(ent, name);
    }

    IBehaviorScript* ScriptFactory::Get(Entity ent, std::string_view name) const
    {
        if (!ecs::Script::Contains(ent))
            return nullptr;

        return ecs::Script::Get(ent)->AddScript(name);
    }

    void ScriptFactory::Remove(Entity ent, IBehaviorScript* script) const
    {
        if (!ecs::Script::Contains(ent))
            return;

        script->OnDestroy();
        ecs::Script::Get(ent)->RemoveScript(script);   
    }

    IBehaviorScript* ScriptFactory::Create(std::string_view name) const
    {
        auto it = _registry.find(name);
        if (it != _registry.end())
        {
            auto p  = it->second.create();
            p->info = &it->second;
            return p;
        }
        return nullptr;
    }

} // namespace fin
