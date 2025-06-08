#pragma once

#include "game_api.h"
#include "plugin.hpp"

namespace fin
{
    class CGameAPI : private GameAPI
    {
    public:
        CGameAPI() = default;
        ~CGameAPI() = default;

        static CGameAPI& Get();
        void        InitAPI(GameAPI*);
        const char* ClassName() const;
        uint32_t    GetVersion() const;

    private:
        static CGameAPI _instance;
    };


    inline CGameAPI& CGameAPI::Get()
    {
        return CGameAPI::_instance;
    }

    inline void CGameAPI::InitAPI(GameAPI* api)
    {
        GameAPI() = *api;
    }

    inline const char* CGameAPI::ClassName() const
    {
        return GameAPI::ClassName();
    }

    inline uint32_t CGameAPI::GetVersion() const
    {
        return GameAPI::version;
    }
}
