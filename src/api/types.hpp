#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

#include <cstdint>
#include <string_view>
#include <vector>
#include <algorithm>
#include <entt.hpp>
#include "msgvar.hpp"
#include "math_utils.hpp"

#define FINCFN(name) (*name)

namespace fin
{
    struct ComponentInfo;
    struct IComponent;
    struct IScriptComponent;
    class IGamePlugin;
    class Register;
    class ObjectSceneLayer;
} // namespace fin

namespace ImGui
{
    struct CanvasParams;
} // namespace ImGui


using Entity           = entt::entity;
using ComponentId      = uint32_t;
using StringView       = std::string_view;
using RegisterHandle   = fin::Register*;
using PluginHandle     = fin::IGamePlugin*;
using SparseSet        = entt::sparse_set;

struct GamePluginInfo
{
    StringView name;        // Plugin name
    StringView description; // Plugin description
    StringView author;      // Plugin author
    StringView version;     // Plugin version
};

struct GameAPI
{
    uint32_t            version; // API_VERSION
    RegisterHandle      registry;
    StringView          classname;
    fin::ComponentInfo* FINCFN(GetComponentInfoByType)(GameAPI& self, StringView type);
    fin::ComponentInfo* FINCFN(GetComponentInfoById)(GameAPI& self, StringView id);
    bool                FINCFN(RegisterComponentInfo)(GameAPI& self, fin::ComponentInfo* info);
};
