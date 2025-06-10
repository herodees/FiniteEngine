#include "game.hpp"
#include "plugin.hpp"

#ifdef BUILDING_DLL

namespace fin
{
    GameAPI gGameAPI = {};
}

extern "C"
{
    LIB_EXPORT fin::IGamePlugin* CreateGamePluginProc(GameAPI* api)
    {
        fin::gGameAPI = *api;
        fin::RegisterBuiltinComponents();
        return fin::IGamePluginFactory::InternalCreate();
    }

    LIB_EXPORT GamePluginInfo* GetGamePluginInfoProc()
    {
        return fin::IGamePluginFactory::InternalInfo();
    }

} // extern "C"

#else // BUILDING_DLL

#endif
