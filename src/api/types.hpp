#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

#include <cstdint>
#include <string_view>
#include <vector>
#include <entt.hpp>

#define FINCFN(name) (*name)

struct ComponentInfo;
using Entity = entt::entity; // Entity is represented by a 32-bit unsigned integer
using ComponentId = uint32_t;
using StringView = std::string_view;
using RegistryHandle = entt::registry*;
using ComponentsHandle = std::vector<ComponentInfo*>*; 
using SparseSet = entt::sparse_set;

namespace fin
{
    class IGamePlugin;
} // namespace fin


struct GamePluginInfo
{
    StringView name;        // Plugin name
    StringView description; // Plugin description
    StringView author;      // Plugin author
    StringView version;     // Plugin version
};

enum ComponentFlags_
{
    ComponentFlags_Default           = 0,      // Default component flags
    ComponentFlags_Private           = 1 << 1, // Component is private and should not be shown in the component list
    ComponentFlags_NoWorkspaceEditor = 1 << 2, // Component should not be shown in the workspace editor
    ComponentFlags_NoEditor          = 1 << 3, // Component should not be shown in the editor at all
    ComponentFlags_NoPrefab          = 1 << 4,
};

using ComponentFlags = uint32_t;

struct ComponentInfo
{
    void* Emplace(Entity ent) const
    {
        this->storage->emplace(ent);
        return Get(ent);
    }

    void* Get(Entity ent) const
    {
        return this->storage->get(ent);
    }

    bool Contains(Entity ent) const
    {
        return this->storage->contains(ent);
    }

    void Remove(Entity ent) const
    {
        this->storage->remove(ent);
    }

    std::string_view  name;
    std::string_view  id;
    std::string_view  label;
    uint32_t          index   = 0;
    ComponentFlags    flags   = 0;       // flags for the component
    SparseSet*        storage = nullptr; // owned only if not external
    fin::IGamePlugin* owner   = nullptr; // owner plugin of the component
};

template <typename T>
struct ComponentInfoStorage : ComponentInfo
{
    entt::storage<T> set;
};

struct GameAPI
{
    uint32_t            version; // API_VERSION
    RegistryHandle      registry;
    ComponentsHandle    components;
    StringView          classname;
    ComponentInfo*      FINCFN(GetComponentInfo)(GameAPI& self, StringView name);
    bool                FINCFN(RegisterComponentInfo)(GameAPI& self, ComponentInfo* info);
};
