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
        static ComponentId component_id; // Unique ID for the component type
        static SparseSet*  storage;
    };

    template <typename T>
    ComponentId ScriptTraits<T>::component_id{};
    template <typename T>
    SparseSet* ScriptTraits<T>::storage{};

    class CGameAPI : private GameAPI
    {
    public:
        CGameAPI()  = default;
        ~CGameAPI() = default;

        static CGameAPI& Get();
        void             InitAPI(GameAPI*);
        StringView       ClassName() const;
        uint32_t         GetVersion() const;
        bool             RegisterComponentInfo(ComponentInfo* info);
        ComponentInfo*   GetComponentInfo(StringView name);

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

    inline StringView CGameAPI::ClassName() const
    {
        return GameAPI::ClassName();
    }

    inline uint32_t CGameAPI::GetVersion() const
    {
        return GameAPI::version;
    }

    inline bool CGameAPI::RegisterComponentInfo(ComponentInfo* info)
    {
        return GameAPI::RegisterComponentInfo(static_cast<GameAPI&>(*this), info);
    }

    inline ComponentInfo* CGameAPI::GetComponentInfo(StringView name)
    {
        return GameAPI::GetComponentInfo(static_cast<GameAPI&>(*this), name);
    }

} // namespace fin
