#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

#include <cstdint>
#include <string_view>

#define FINCFN(name) (*name)

using Entity = uint32_t; // Entity is represented by a 32-bit unsigned integer
using ComponentId = uint32_t;
using StringView = std::string_view;
using RegistryHandle = void*; 



constexpr std::uint32_t fnv1a_hash(StringView str)
{
    std::uint32_t hash = 2166136261u; // FNV offset basis
    std::size_t   size = str.size();  // FNV offset basis
    for (std::size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<std::uint32_t>(str[i]);
        hash *= 16777619u; // FNV prime
    }
    return hash;
}



template <typename T>
constexpr StringView type_name()
{
#if defined(__clang__) || defined(__GNUC__)
    return StringView(__PRETTY_FUNCTION__);
#elif defined(_MSC_VER)
    return StringView(__FUNCSIG__);
#else
    static_assert(false, "Unsupported compiler");
#endif
    return StringView("");
}

template <typename T>
constexpr ComponentId type_hash()
{
    return fnv1a_hash(type_name<T>());
}

struct GamePluginInfo
{
    const char* name;        // Plugin name
    const char* description; // Plugin description
    const char* author;      // Plugin author
    const char* version;     // Plugin version
};

struct GameAPI
{
    uint32_t       version; // API_VERSION
    const char*    FINCFN(ClassName)(void);
    RegistryHandle registry;
    ComponentId    FINCFN(RegisterComponent)(RegistryHandle reg, StringView name, uint32_t size);
    void*          FINCFN(GetComponent)(RegistryHandle reg, Entity entity, ComponentId component);
    void*          FINCFN(EmplaceComponent)(RegistryHandle reg, Entity entity, ComponentId component);
};
