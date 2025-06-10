#pragma once

#include "types.hpp"
#include <algorithm>

namespace fin
{
    class Register
    {
    public:
        using entity_traits  = entt::entt_traits<Entity>;
        using version_type   = typename entity_traits::version_type;
        using size_type      = std::size_t;
        using componets_type = std::vector<ComponentInfo*>;

        Register()                           = default;
        Register(const Register&)            = delete;
        Register& operator=(const Register&) = delete;
        Register(Register&&)                 = default;
        Register& operator=(Register&&)      = default;
        ~Register()                          = default;

        [[nodiscard]] Entity Create();
        [[nodiscard]] bool   Valid(const Entity entt) const;
        version_type         Release(const Entity entt);
        version_type         Release(const Entity entt, const version_type version);
        version_type         Destroy(const Entity entt);
        version_type         Destroy(const Entity entt, const version_type version);
        void                 Emplace(const Entity entt, ComponentId cmp);
        void*                Get(const Entity entt, ComponentId cmp);
        bool                 Contains(const Entity entt, ComponentId cmp) const;
        void                 Remove(const Entity entt, ComponentId cmp);

        template <typename C>
        [[nodiscard]] C& Emplace(const Entity entt);
        template <typename C>
        [[nodiscard]] C& Get(const Entity entt);
        template <typename C>
        [[nodiscard]] bool Contains(const Entity entt) const;
        template <typename C>
        void Remove(const Entity entt);

        void            AddComponentInfo(ComponentInfo* info);
        componets_type& GetComponents();
        ComponentInfo*  GetComponentInfoByType(StringView name) const;
        ComponentInfo*  GetComponentInfoById(StringView name) const;
    private:
        Entity       GenerateIdentifier(const std::size_t pos) noexcept;
        Entity       RecycleIdentifier() noexcept;
        version_type ReleaseEntity(const Entity entt, const typename entity_traits::version_type version);

        std::vector<Entity> _pool;
        componets_type      _components;
        Entity              _free = entt::null;
    };

    inline Entity Register::Create()
    {
        return (_free == entt::null) ? _pool.emplace_back(GenerateIdentifier(_pool.size())) : RecycleIdentifier();
    }

    inline bool Register::Valid(const Entity entt) const
    {
        const auto pos = size_type(entity_traits::to_entity(entt));
        return (pos < _pool.size() && _pool[pos] == entt);
    }

    inline Register::version_type Register::Release(const Entity entt)
    {
        return Release(entt, static_cast<version_type>(entity_traits::to_version(entt) + 1u));
    }

    inline Register::version_type Register::Release(const Entity entt, const version_type version)
    {
        ENTT_ASSERT(Valid(entt), "Invalid identifier");

        return ReleaseEntity(entt, version);
    }

    inline Register::version_type Register::Destroy(const Entity entt)
    {
        return Destroy(entt, static_cast<version_type>(entity_traits::to_version(entt) + 1u));
    }

    inline Register::version_type Register::Destroy(const Entity entt, const version_type version)
    {
        for (auto& component : _components)
        {
            if (component->storage->contains(entt))
            {
                component->storage->remove(entt);
            }
        }
        return Release(entt, version);
    }

    inline void Register::Emplace(const Entity entt, ComponentId cmp)
    {
        _components[cmp]->storage->emplace(entt);
    }

    inline void* Register::Get(const Entity entt, ComponentId cmp)
    {
        return _components[cmp]->storage->get(entt);
    }

    inline bool Register::Contains(const Entity entt, ComponentId cmp) const
    {
        return _components[cmp]->storage->contains(entt);
    }

    inline void Register::Remove(const Entity entt, ComponentId cmp)
    {
        _components[cmp]->storage->remove(entt);
    }

    inline void Register::AddComponentInfo(ComponentInfo* info)
    {
        ENTT_ASSERT(info != nullptr, "Invalid info");
        ENTT_ASSERT(std::find(_components.cbegin(), _components.cend(), info) == _components.cend(), "Component alredy exists");

        info->index = uint32_t(_components.size());
        _components.push_back(info);
    }

    inline ComponentInfo* Register::GetComponentInfoByType(StringView name) const
    {
        for (auto& el : _components)
        {
            if (el->name == name)
                return el;
        }
        return nullptr;
    }

    inline ComponentInfo* Register::GetComponentInfoById(StringView name) const
    {
        for (auto& el : _components)
        {
            if (el->id == name)
                return el;
        }
        return nullptr;
    }

    inline Entity Register::GenerateIdentifier(const std::size_t pos) noexcept
    {
        ENTT_ASSERT(pos < entity_traits::to_entity(entt::null), "No entities available");
        return entity_traits::combine(static_cast<typename entity_traits::entity_type>(pos), {});
    }

    inline Entity Register::RecycleIdentifier() noexcept
    {
        ENTT_ASSERT(_free != entt::null, "No entities available");
        const auto curr = entity_traits::to_entity(_free);
        _free           = entity_traits::combine(entity_traits::to_integral(_pool[curr]), entt::tombstone);
        return (_pool[curr] = entity_traits::combine(curr, entity_traits::to_integral(_pool[curr])));
    }


    inline Register::version_type Register::ReleaseEntity(const Entity entt, const typename entity_traits::version_type version)
    {
        const typename entity_traits::version_type vers = version + (version == entity_traits::to_version(entt::tombstone));
        _pool[entity_traits::to_entity(entt)] = entity_traits::construct(entity_traits::to_integral(_free), vers);
        _free = entity_traits::combine(entity_traits::to_integral(entt), entt::tombstone);
        return vers;
    }

    inline Register::componets_type& Register::GetComponents()
    {
        return _components;
    }

    template <typename C>
    inline C& Register::Emplace(const Entity entt)
    {
        ENTT_ASSERT(ComponentTraits<C>::component_id >= _components.size(), "Unregistered component");

        return static_cast<ComponentInfoStorage<C>*>(_components[ComponentTraits<C>::component_id]->storage)->set.emplace(entt);
    }

    template <typename C>
    inline C& Register::Get(const Entity entt)
    {
        ENTT_ASSERT(ComponentTraits<C>::component_id >= _components.size(), "Unregistered component");

        return static_cast<ComponentInfoStorage<C>*>(_components[ComponentTraits<C>::component_id]->storage)->set.get(entt);
    }

    template <typename C>
    inline bool Register::Contains(const Entity entt) const
    {
        ENTT_ASSERT(ComponentTraits<C>::component_id >= _components.size(), "Unregistered component");

        return _components[ComponentTraits<C>::component_id]->storage->contains(entt);
    }

    template <typename C>
    inline void Register::Remove(const Entity entt)
    {
        ENTT_ASSERT(ComponentTraits<C>::component_id >= _components.size(), "Unregistered component");

        return static_cast<ComponentInfoStorage<C>*>(_components[ComponentTraits<C>::component_id]->storage)->set.remove(entt);
    }

} // namespace fin
