#include "game.hpp"
#include "plugin.hpp"

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
