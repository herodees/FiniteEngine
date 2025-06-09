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
using ComponentListHandle = std::vector<ComponentInfo>*; 
using SparseSet = entt::sparse_set;

struct GamePluginInfo
{
    StringView name;        // Plugin name
    StringView description; // Plugin description
    StringView author;      // Plugin author
    StringView version;     // Plugin version
};

struct ComponentInfo
{
    std::string_view name;
    std::string_view id;
    std::string_view label;
    uint32_t         size     = 0;
    uint32_t         index    = 0;
    SparseSet*       storage  = nullptr; // owned only if !external
    bool             external = false;   // if true, we don’t own the storage
};

struct GameAPI
{
    uint32_t            version; // API_VERSION
    RegistryHandle      registry;
    ComponentListHandle components;
    StringView          FINCFN(ClassName)(void);
    ComponentId         FINCFN(RegisterComponent)(GameAPI& self, StringView name, uint32_t size);
    void*               FINCFN(GetComponent)(GameAPI& self, Entity entity, ComponentId component);
    void*               FINCFN(EmplaceComponent)(GameAPI& self, Entity entity, ComponentId component);
    ComponentInfo*      FINCFN(GetComponentInfo)(GameAPI& self, StringView name);
    bool                FINCFN(RegisterComponentInfo)(GameAPI& self, ComponentInfo* info);
};
