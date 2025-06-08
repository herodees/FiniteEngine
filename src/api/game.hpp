#pragma once

#include "types.hpp"
#include <algorithm>

namespace fin
{
    class ScriptComponent
    {
    public:
        ScriptComponent()                   = default;
        virtual ~ScriptComponent()          = default;
        virtual void Init(Entity entity)     = 0; // Initialize the component for the given entity
        virtual void Update(float deltaTime) = 0; // Update the component logic
    };




    template <typename T>
    struct ScriptTraits 
    {
        static ComponentId __component_id; // Unique ID for the component type
    };

    template <typename T>
    ComponentId ScriptTraits<T>::__component_id{};



    class CGameAPI : private GameAPI
    {
    public:
        CGameAPI() = default;
        ~CGameAPI() = default;

        static CGameAPI& Get();
        void        InitAPI(GameAPI*);
        const char* ClassName() const;
        uint32_t    GetVersion() const;

        template <typename C>
        uint32_t RegisterComponent();

        template <typename C>
        C* GetComponent(Entity entity);

    private:
        static CGameAPI _instance;
    };

    inline CGameAPI& CGameAPI::Get()
    {
        return CGameAPI::_instance;
    }

    inline void CGameAPI::InitAPI(GameAPI* api)
    {
        *static_cast<GameAPI*>(this) = *api; // Copy the GameAPI structure to this instance
    }

    inline const char* CGameAPI::ClassName() const
    {
        return GameAPI::ClassName();
    }

    inline uint32_t CGameAPI::GetVersion() const
    {
        return GameAPI::version;
    }

    template <typename C>
    inline uint32_t CGameAPI::RegisterComponent()
    {
        static_assert(std::is_base_of_v<ScriptComponent, C>, "Component must derive from ScriptComponent");

        ScriptTraits<C>::__component_id = GameAPI::RegisterComponent(GameAPI::registry, type_name<C>(), (uint32_t)sizeof(C));

        return ScriptTraits<C>::__component_id;
    }

    template <typename C>
    inline C* CGameAPI::GetComponent(Entity entity)
    {
        static_assert(std::is_base_of_v<ScriptComponent, C>, "Component must derive from ScriptComponent");

        return static_cast<C*>(GameAPI::GetComponent(GameAPI::registry, entity, ScriptTraits<C>::__component_id));
    }

} // namespace fin
