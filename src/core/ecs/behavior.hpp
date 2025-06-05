#pragma once

#include "include.hpp"

namespace fin
{
    enum BehaviorFlags_
    {
        BehaviorFlags_Default = 0,
        BehaviorFlags_Private = 1 << 0, // Private behavior, not visible in editor
    };

    using BehaviorFlags = uint32_t;


    struct BehaviorData
    {
        SparseSet*       storage;
        std::string_view id;
        std::string_view label;
        BehaviorFlags    flags;
        void (*update)(const SparseSet& set, float dt);
    };

    struct BehaviorTag
    {
    };



    class IBehaviorScript
    {
        EntityHandle _self;
    public:
        virtual ~IBehaviorScript() {};
        virtual void OnStart() {};
        virtual void OnUpdate(float dt) = 0;
        virtual void OnDestroy() {};
        virtual bool OnEdit()
        {
            return false;
        };
        virtual void                Load(msg::Var& ar) {};
        virtual void                Save(msg::Var& ar) const {};
        virtual std::string_view    GetType() const  = 0;
        virtual std::string_view    GetLabel() const = 0;
        virtual const BehaviorData& GetData() const  = 0;

        EntityHandle&               Self();
    };



    template <typename C, std::string_literal ID, std::string_literal LABEL>
    struct Behavior : public IBehaviorScript, BehaviorTag
    {
    public:
        using self_type      = Behavior<C, ID, LABEL>;
        using Storage        = entt::storage_for_t<C>;
        ~Behavior() override = default;

        static Storage&      GetStorage();
        static void          Emplace(Entity e);
        static void          Remove(Entity e);
        static bool          Contains(Entity e);
        static C*            Get(Entity e);

        void                 OnUpdate(float dt) override = 0;
        std::string_view     GetType() const override;
        std::string_view     GetLabel() const override;
        const BehaviorData&  GetData() const override;
        static BehaviorData& GetData(Registry& reg, BehaviorFlags flags);
        static void          Update(const SparseSet& set, float dt);

    private:
        static BehaviorData _data;
    };



    template <typename C, std::string_literal ID, std::string_literal LABEL>
    BehaviorData Behavior<C, ID, LABEL>::_data{};

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline BehaviorData& Behavior<C, ID, LABEL>::GetData(Registry& reg, BehaviorFlags flags)
    {
        _data.storage  = &reg.storage<C>();
        _data.id       = ID.value;
        _data.label    = LABEL.value;
        _data.flags    = flags;
        _data.update   = &C::Update;
        return _data;
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline void Behavior<C, ID, LABEL>::Update(const SparseSet& set, float dt)
    {
        if (set.size() > _data.storage->size())
        {
            for (auto ent : *_data.storage)
            {
                if (!set.contains(ent))
                    continue;
                Get(ent)->OnUpdate(dt);
            }
        }
        else
        {
            for (auto ent : set)
            {
                if (!_data.storage->contains(ent))
                    continue;
                Get(ent)->OnUpdate(dt);
            }
        }
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline Behavior<C, ID, LABEL>::Storage& Behavior<C, ID, LABEL>::GetStorage()
    {
        return static_cast<Storage&>(*_data.storage);
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline C* Behavior<C, ID, LABEL>::Get(Entity e)
    {
        return static_cast<C*>(_data.storage->get(e));
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline bool Behavior<C, ID, LABEL>::Contains(Entity e)
    {
        return _data.storage->contains(e);
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline void Behavior<C, ID, LABEL>::Remove(Entity e)
    {
        _data.storage->remove(e);
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline void Behavior<C, ID, LABEL>::Emplace(Entity e)
    {
        _data.storage->emplace(e);
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline std::string_view Behavior<C, ID, LABEL>::GetLabel() const
    {
        return _data.label;
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline const BehaviorData& Behavior<C, ID, LABEL>::GetData() const
    {
        return _data;
    }

    inline EntityHandle& IBehaviorScript::Self()
    {
        return _self;
    }

    template <typename C, std::string_literal ID, std::string_literal LABEL>
    inline std::string_view Behavior<C, ID, LABEL>::GetType() const
    {
        return _data.id;
    }
}

