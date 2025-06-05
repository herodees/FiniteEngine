#pragma once

#include "include.hpp"

namespace fin
{
    struct IBehaviorScript; // Forward declaration
    class ObjectSceneLayer; // Forward declaration
    class Scene;            // Forward declaration

    namespace ecs
    {
        struct Script;  // Forward declaration
        class Behavior; // Forward declaration
    } // namespace ecs

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



    class IBehaviorScript
    {
    protected:
        friend class ScriptFactory;
        friend struct ecs::Script;
        friend struct ecs::Behavior;
        IBehaviorScript*    next{};
        const BehaviorInfo* info{};
        Entity              entity{};

    public:
        virtual ~IBehaviorScript() {};
        virtual void OnStart() {};
        virtual void OnUpdate(float dt) = 0;
        virtual void OnDestroy() {};
        virtual bool OnEdit()
        {
            return false;
        };
        virtual void Load(msg::Var& ar) {};
        virtual void Save(msg::Var& ar) const {};
        virtual std::string_view GetType() const = 0;
        virtual std::string_view GetLabel() const = 0;
    };



    template <typename CLASS, std::string_literal ID, std::string_literal LABEL>
    class Behavior : public IBehaviorScript
    {
    public:
        Behavior() = default;

        std::string_view GetType() const
        {
            return ID.data;
        };
        std::string_view GetLabel() const
        {
            return LABEL.data;
        };

        template <typename C>
        C* GetComponent();
        template <typename C>
        void AddComponent();
        template <typename C>
        void RemoveComponent();
        template <typename C>
        bool HasComponent() const;

        template <typename B>
        B* GetBehavior();
        template <typename B>
        B* AddBehavior();
        template <typename B>
        void RemoveBehavior(B* bh);
        void RemoveBehavior(std::string_view name);
        template <typename B>
        bool HasBehavior() const;
        bool HasBehavior(std::string_view name) const;

        ObjectSceneLayer*   GetLayer();
        Scene*              GetScene();
        Entity              GetEntity() const;
        const BehaviorInfo* GetInfo() const;

        Vec2f GetPosition() const;
        void  SetPosition(Vec2f pos);
        bool  IsActive() const;
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
