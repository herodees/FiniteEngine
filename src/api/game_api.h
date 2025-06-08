#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

#include <cstdint>

#define FINCFN(name) (*name)

struct GamePluginInfo
{
    const char* name;        // Plugin name
    const char* description; // Plugin description
    const char* author;      // Plugin author
    const char* version;     // Plugin version
};

struct GameAPI
{
    uint32_t version; // API_VERSION
    const char* FINCFN(ClassName)(void);
};
