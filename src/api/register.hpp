#pragma once

#include "types.hpp"
#include <algorithm>

namespace fin
{
    class Register
    {
    public:
        using entity_traits = entt::entt_traits<Entity>;
        using version_type = typename entity_traits::version_type;
        using size_type = std::size_t;
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

        void AddComponentInfo(ComponentInfo* info);
        componets_type& GetComponents();

    private:
        Entity GenerateIdentifier(const std::size_t pos) noexcept
        {
            ENTT_ASSERT(pos < entity_traits::to_entity(entt::null), "No entities available");
            return entity_traits::combine(static_cast<typename entity_traits::entity_type>(pos), {});
        }

        Entity RecycleIdentifier() noexcept
        {
            ENTT_ASSERT(_free != entt::null, "No entities available");
            const auto curr = entity_traits::to_entity(_free);
            _free           = entity_traits::combine(entity_traits::to_integral(_pool[curr]), entt::tombstone);
            return (_pool[curr] = entity_traits::combine(curr, entity_traits::to_integral(_pool[curr])));
        }

        version_type ReleaseEntity(const Entity entt, const typename entity_traits::version_type version)
        {
            const typename entity_traits::version_type vers = version +
                                                              (version == entity_traits::to_version(entt::tombstone));
            _pool[entity_traits::to_entity(entt)] = entity_traits::construct(entity_traits::to_integral(_free), vers);
            _free = entity_traits::combine(entity_traits::to_integral(entt), entt::tombstone);
            return vers;
        }

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
            if (component->Contains(entt))
            {
                component->Remove(entt);
            }
        }
        return Release(entt, version);
    }

    inline void Register::AddComponentInfo(ComponentInfo* info)
    {
        _components.push_back(info);
    }

    inline Register::componets_type& Register::GetComponents()
    {
        return _components;
    }

} // namespace fin
