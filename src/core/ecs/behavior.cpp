#include "behavior.hpp"
#include "base.hpp"

namespace fin
{
    IBehaviorScript* ScriptFactory::Enplace(Entity ent, std::string_view name) const
    {
        if (!ecs::Script::contains(ent))
            ecs::Script::emplace(ent);

        auto* s = Create(name);
        if (s == nullptr)
            return nullptr;

        auto* scr = ecs::Script::get(ent);
        scr->add_script(s);
        s->OnStart(ent);
        return s;
    }

    bool ScriptFactory::Contains(Entity ent, std::string_view name) const
    {
        return Get(ent, name);
    }

    IBehaviorScript* ScriptFactory::Get(Entity ent, std::string_view name) const
    {
        if (!ecs::Script::contains(ent))
            return nullptr;

        return ecs::Script::get(ent)->get_script(name);
    }

    void ScriptFactory::Remove(Entity ent, IBehaviorScript* script) const
    {
        if (!ecs::Script::contains(ent))
            return;

        script->OnDestroy(ent);
        ecs::Script::get(ent)->remove_script(script);   
    }

    IBehaviorScript* ScriptFactory::Create(std::string_view name) const
    {
        auto it = _registry.find(name);
        if (it != _registry.end())
        {
            auto p       = it->second();
            p->type_name = it->first;
            return p;
        }
        return nullptr;
    }

} // namespace fin
