#pragma once

#include "types.hpp"

namespace fin
{
    extern GameAPI gGameAPI;

    enum ComponentsFlags_
    {
        ComponentsFlags_Default           = 0,      // Default component flags
        ComponentsFlags_Private           = 1 << 1, // Component is private and should not be shown in the component list
        ComponentsFlags_NoWorkspaceEditor = 1 << 2, // Component should not be shown in the workspace editor
        ComponentsFlags_NoEditor          = 1 << 3, // Component should not be shown in the editor at all
        ComponentsFlags_NoPrefab          = 1 << 4,
    };
    using ComponentsFlags = uint32_t;


    struct ArchiveParams2
    {
        Entity   entity; // Entity being serialized/deserialized
        msg::Var data;   // Archive variable to serialize/deserialize data
    };



    struct IComponent
    {
        virtual void OnSerialize(ArchiveParams2& ar) {};
        virtual bool OnDeserialize(ArchiveParams2& ar)
        {
            return false;
        };
        virtual bool OnEdit(Entity ent)
        {
            return false;
        };
        virtual bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
        {
            return false;
        };
    };



    class IScriptComponent : public IComponent
    {
    public:
        IScriptComponent()                   = default;
        virtual ~IScriptComponent()          = default;
        virtual void Init(Entity entity)     = 0; // Initialize the component for the given entity
        virtual void Update(float deltaTime) = 0; // Update the component logic
    };



    struct ComponentInfo
    {
        std::string_view name;
        std::string_view id;
        std::string_view label;
        uint32_t         index       = 0;
        ComponentsFlags  flags       = 0;       // flags for the component
        SparseSet*       storage     = nullptr; // owned only if not external
        PluginHandle     owner       = nullptr; // owner plugin of the component

        [[nodiscard]] IComponent* Get(const Entity ent) const
        {
            return static_cast<IComponent*>(storage->get(ent));
        }

        [[nodiscard]] IComponent* Find(const Entity ent) const
        {
            return storage->contains(ent) ? static_cast<IComponent*>(storage->get(ent)) : nullptr;
        }

        [[nodiscard]] bool Contains(const Entity ent) const
        {
            return storage->contains(ent);
        }

        void Emplace(Entity ent) const
        {
            storage->emplace(ent);
        }

        void Erase(Entity ent) const
        {
            storage->erase(ent);
        }

        void OnSerialize(ArchiveParams2& ar)
        {
            Get(ar.entity)->OnSerialize(ar);
        }

        bool OnDeserialize(ArchiveParams2& ar)
        {
            return Get(ar.entity)->OnDeserialize(ar);
        }

        bool OnEdit(Entity ent)
        {
            if (flags & ComponentsFlags_NoEditor)
            {
                return false; // Component is not editable in the editor
            }
            return Get(ent)->OnEdit(ent);
        }

        bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas)
        {
            if (flags & ComponentsFlags_NoWorkspaceEditor)
            {
                return false; // Component is not editable in the editor
            }
            return Get(ent)->OnEditCanvas(ent, canvas);
        }
    };

    template <typename T>
    struct ComponentInfoStorage : ComponentInfo
    {
        entt::storage<T> set;
    };




    template <typename T>
    struct ComponentTraits
    {
        static ComponentInfo*    info;
        static SparseSet*        set;

        [[nodiscard]] static T& Get(const Entity ent)
        {
            return static_cast<entt::storage<T>*>(set)->get(ent);
        }
        [[nodiscard]] static T* Find(const Entity ent)
        {
            auto* st = static_cast<entt::storage<T>*>(set);
            return st->contains(ent) ? &st->get(ent) : nullptr;
        }
        [[nodiscard]] static bool Contains(const Entity ent)
        {
            return static_cast<entt::storage<T>*>(set)->contains(ent);
        }
        static T& Emplace(Entity ent)
        {
            if constexpr (std::is_constructible_v<T>)
            {
                // If T is Interface, use set->emplace
                return static_cast<entt::storage<T>*>(set)->emplace(ent);
            }
            else
            {
                // Fallback for incomplete or non-constructible types
                set->emplace(ent);
                return *static_cast<T*>(set->get(ent));
            }
        }
        static void Erase(Entity ent)
        {
            static_cast<entt::storage<T>*>(set)->erase(ent);
        }
    };

    template <typename T>
    SparseSet* ComponentTraits<T>::set{};
    template <typename T>
    ComponentInfo* ComponentTraits<T>::info{};



    template <typename C>
    inline ComponentInfo* NewComponentInfo(StringView name, StringView label, ComponentsFlags flags, IGamePlugin* owner = nullptr)
    {
        static_assert(std::is_base_of_v<IComponent, C>, "Component must derive from IComponent");
        static_assert(std::is_default_constructible_v<C>, "Component is not default constructible, or is incomplete");

        if (auto* info = gGameAPI.GetComponentInfoByType(gGameAPI.context, entt::internal::stripped_type_name<C>()))
        {
            throw std::runtime_error("Component already registered!");
        }

        auto* info    = new ComponentInfoStorage<C>();
        info->name    = entt::internal::stripped_type_name<C>();
        info->id      = name;
        info->label   = label.empty() ? name : label;
        info->flags   = flags;
        info->owner   = owner;
        info->storage = &info->set;

        ComponentTraits<C>::info = info;
        ComponentTraits<C>::set  = info->storage;

        return info;
    }

    template <typename C>
    inline bool LoadBuiltinComponent(StringView name)
    {
        if (auto* nfo = gGameAPI.GetComponentInfoById(gGameAPI.context, name))
        {
            ComponentTraits<C>::info = nfo;
            ComponentTraits<C>::set  = nfo->storage;
            return true;
        }
        else
        {
            ENTT_ASSERT(false, "Component is not registered!");
        }
        return false;
    }


    template <typename T>
    concept ComponentType = requires(Entity ent)
    {
        { ComponentTraits<T>::Get(ent) } -> std::same_as<T&>;
        { ComponentTraits<T>::Find(ent) } -> std::same_as<T*>;
        { ComponentTraits<T>::Emplace(ent) } -> std::same_as<T&>;
        { ComponentTraits<T>::Erase(ent) } -> std::same_as<void>;
        { ComponentTraits<T>::Contains(ent) } -> std::same_as<bool>;
    };

    template <ComponentType T>
    [[nodiscard]] T& Get(Entity ent)
    {
        return ComponentTraits<T>::Get(ent);
    }

    template <ComponentType T>
    [[nodiscard]] T* Find(Entity ent)
    {
        return ComponentTraits<T>::Find(ent);
    }

    template <ComponentType T>
    T& Emplace(Entity ent)
    {
        return ComponentTraits<T>::Emplace(ent);
    }

    template <ComponentType T>
    void Erase(Entity ent)
    {
        ComponentTraits<T>::Erase(ent);
    }

    template <ComponentType T>
    bool Contains(Entity ent)
    {
        return ComponentTraits<T>::Contains(ent);
    }



    template <typename... Ts>
    class View
    {
        static constexpr std::size_t N = sizeof...(Ts);
        std::array<const SparseSet*, N> all_sets{};
    public:
        class Iterator
        {
        public:
            using iterator_type  = SparseSet::iterator;
            using const_iterator = SparseSet::const_iterator;
            using pointer        = typename iterator_type::pointer;
            using reference      = typename iterator_type::reference;

            [[nodiscard]] bool valid() const noexcept
            {
                return ((N != 0u) || (*it != entt::tombstone)) &&
                       std::apply([entt = *it](const auto*... curr) { return (curr->contains(entt) && ...); }, pools);
            }

            constexpr Iterator() noexcept : it{}, last{}, pools{}
            {
            }

            Iterator(iterator_type curr, iterator_type to, std::array<const SparseSet*, N> all_of) noexcept :
            it{curr},
            last{to},
            pools{all_of}
            {
                while (it != last && !valid())
                {
                    ++it;
                }
            }

            Iterator& operator++() noexcept
            {
                while (++it != last && !valid())
                {
                }
                return *this;
            }

            Iterator operator++(int) noexcept
            {
                Iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] pointer operator->() const noexcept
            {
                return &*it;
            }

            [[nodiscard]] reference operator*() const noexcept
            {
                return *operator->();
            }

            [[nodiscard]] constexpr bool operator==(const Iterator& other) const noexcept
            {
                return it == other.it;
            }

            [[nodiscard]] constexpr bool operator!=(const Iterator& other) const noexcept
            {
                return !(*this == other);
            }

        private:
            iterator_type             it;
            iterator_type             last;
            std::array<const SparseSet*, N> pools;
        };


        View()
        {
            std::array<const SparseSet*, N> sets = {ComponentTraits<Ts>::set...};
            all_sets                       = {ComponentTraits<Ts>::set...};

            auto min_it = std::min_element(all_sets.begin(),
                                           all_sets.end(),
                                           [](const SparseSet* a, const SparseSet* b) { return a->size() < b->size(); });
            if (min_it != all_sets.begin())
            {
                std::iter_swap(all_sets.begin(), min_it);
            }

        }
        [[nodiscard]] bool Contains(const Entity entt) const noexcept
        {
            return std::apply([entt](const auto*... curr) { return (curr->contains(entt) && ...); }, all_sets);
        }
        [[nodiscard]] Iterator begin() const noexcept
        {
            return Iterator{all_sets[0]->begin(), all_sets[0]->end(), all_sets};
        }
        [[nodiscard]] Iterator end() const noexcept
        {
            return Iterator{all_sets[0]->end(), all_sets[0]->end(), all_sets};
        }
        [[nodiscard]] size_t size_hint() const noexcept
        {
            return all_sets[0]->size();
        }
    };
} // namespace fin
