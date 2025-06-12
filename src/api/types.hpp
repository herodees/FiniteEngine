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

namespace fin
{
    struct ArchiveParams2;
    struct ComponentInfo;
    struct IComponent;
    struct IScriptComponent;
    struct IComponent;
    class IScriptComponent;
    class IGamePlugin;
    class Application;
    class ObjectSceneLayer;
    class Renderer;
    class Layer;
    class ObjectLayer;
} // namespace fin

namespace ImGui
{
    struct CanvasParams;
} // namespace ImGui

using Entity         = entt::entity;
using ComponentId    = uint32_t;
using StringView     = std::string_view;
using AppHandle      = fin::Application*;
using PluginHandle   = fin::IGamePlugin*;
using SparseSet      = entt::sparse_set;

struct GamePluginInfo
{
    StringView name;        // Plugin name
    StringView description; // Plugin description
    StringView author;      // Plugin author
    StringView version;     // Plugin version
    StringView guid;
};

#define FINCFN(name) (*name)

struct GameAPI
{
    uint32_t            version; // API_VERSION
    AppHandle           context;
    fin::ComponentInfo* FINCFN(GetComponentInfoByType)(AppHandle self, StringView type);
    fin::ComponentInfo* FINCFN(GetComponentInfoById)(AppHandle self, StringView id);
    bool                FINCFN(RegisterComponentInfo)(AppHandle self, fin::ComponentInfo* info);
    Entity              FINCFN(FindNamedEntity)(AppHandle self, StringView name);
    bool                FINCFN(SetNamedEntity)(AppHandle self, Entity ent, StringView name);
    void                FINCFN(ClearNamededEntity)(AppHandle self, StringView name);
    Entity              FINCFN(GetOldEntity)(AppHandle self, Entity oldent);
    Entity              FINCFN(CreateEntity)(AppHandle self);
    void                FINCFN(DestroyEntity)(AppHandle self, Entity ent);
    bool                FINCFN(ValidEntity)(AppHandle self, Entity ent);
    fin::Layer*         FINCFN(FindLayer)(AppHandle self, StringView name);
};

struct ImguiAPI
{
    uint32_t FINCFN(GetID)(const char* id);
    void     FINCFN(PushID)(const char* id);
    void     FINCFN(PushPtrID)(const void* id);
    void     FINCFN(PopID)();
    void     FINCFN(Text)(const char* fmt, ...);
    bool     FINCFN(Button)(const char* id, fin::Vec2f size);
    bool     FINCFN(Select)(const char* id, bool selected);
    bool     FINCFN(Checkbox)(const char* id, bool* selected);
    bool     FINCFN(BeginCombo)(const char* id, const char* selected);
    void     FINCFN(EndCombo)();
};

