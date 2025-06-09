#include "game.hpp"
#include "plugin.hpp"

#ifdef BUILDING_DLL

extern "C"
{
    fin::CGameAPI fin::CGameAPI::_instance{};

    LIB_EXPORT fin::IGamePlugin* CreateGamePluginProc(GameAPI* api)
    {
        fin::CGameAPI::Get().InitAPI(api);
        return fin::IGamePluginFactory::InternalCreate();
    }

    LIB_EXPORT GamePluginInfo* GetGamePluginInfoProc()
    {
        return fin::IGamePluginFactory::InternalInfo();
    }

} // extern "C"

#else // BUILDING_DLL

fin::CGameAPI fin::CGameAPI::_instance{};

#endif
