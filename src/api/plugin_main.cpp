#include "game.hpp"
#include "plugin.hpp"

#ifndef BUILDING_DLL
#if defined(_WINDLL) || defined(_USRDLL) || defined(__PIC__) || defined(__pic__)
#define BUILDING_DLL 1
#endif
#endif

#ifdef BUILDING_DLL

namespace fin
{
    GameAPI gGameAPI = {};
    ImguiAPI gImguiAPI = {};
}

extern "C"
{
    LIB_EXPORT bool InitGamePluginProc(GameAPI* api, ImguiAPI* iapi)
    {
        fin::gGameAPI = *api;
        fin::gImguiAPI = *iapi;
        return fin::RegisterBuiltinComponents();
    }

    LIB_EXPORT fin::IGamePlugin* CreateGamePluginProc()
    {
        return fin::IGamePluginFactory::InternalCreate();
    }

    LIB_EXPORT GamePluginInfo* GetGamePluginInfoProc()
    {
        return fin::IGamePluginFactory::InternalInfo();
    }

} // extern "C"

#else // BUILDING_DLL

#endif
