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



    struct ComponentInfo
    {
        std::string_view name;
        std::string_view id;
        std::string_view label;
        uint32_t         index       = 0;
        ComponentsFlags  flags       = 0;       // flags for the component
        SparseSet*       storage     = nullptr; // owned only if not external
        PluginHandle     owner       = nullptr; // owner plugin of the component

        IComponent* Get(const Entity ent) const
        {
            return static_cast<IComponent*>(storage->get(ent));
        }

        bool Contains(const Entity ent) const
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
    };



    template <typename T>
    struct ComponentInfoStorage : ComponentInfo
    {
        entt::storage<T> set;
    };


    struct ArchiveParams2
    {
        msg::Var& data;   // Archive variable to serialize/deserialize data
        Entity    entity; // Entity being serialized/deserialized
    };
        


    struct IComponent
    {
        virtual void OnSerialize(ArchiveParams2& ar) {};
        virtual bool OnDeserialize(ArchiveParams2& ar){ return false;};
        virtual bool OnEdit(Entity ent){ return false;};
        virtual bool OnEditCanvas(Entity ent, ImGui::CanvasParams& canvas){ return false;};
    };



    class IScriptComponent : public IComponent
    {
    public:
        IScriptComponent()                   = default;
        virtual ~IScriptComponent()          = default;
        virtual void Init(Entity entity)     = 0; // Initialize the component for the given entity
        virtual void Update(float deltaTime) = 0; // Update the component logic
    };



    template <typename T>
    struct ComponentTraits
    {
        static ComponentInfo*    info;
        static SparseSet*        set;

        static T& Get(const Entity ent)
        {
            return static_cast<entt::storage<T>*>(set)->get(ent);
        }

        static bool Contains(const Entity ent)
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
    using CT = ComponentTraits<T>;

    template <typename T>
    SparseSet* ComponentTraits<T>::set{};
    template <typename T>
    ComponentInfo* ComponentTraits<T>::info{};


} // namespace fin
