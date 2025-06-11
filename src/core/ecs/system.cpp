#include "system.hpp"
#include "core/scene.hpp"

namespace fin
{
    System::System(Scene& s) : _scene(s)
    {
    }

    Register& System::GetRegister()
    {
        return _scene.GetFactory().GetRegister();
    }

    Scene& System::GetScene()
    {
        return _scene;
    }

    std::string_view System::GetName() const
    {
        return _info->name;
    }

    SystemInfo* System::GetSystemInfo() const
    {
        return _info;
    }

    bool System::ShouldRunSystem() const
    {
        return !(_flags & SystemFlags_Disabled);
    }

    bool System::ImguiSetup()
    {
        return false;
    }

    SystemManager::SystemManager(Scene& scene) : _scene(scene)
    {
    }

    SystemManager::~SystemManager()
    {
        std::for_each(_systems.begin(), _systems.end(), [](System* system) { delete system; });
    }

    void SystemManager::AddDefaults()
    {
        for (auto& it : _system_map)
        {
            if (!(it.second.flags & SystemFlags_Extra))
            {
                auto* sys = it.second.create(_scene, &it.second);
                sys->_index = int32_t(_systems.size());
                _systems.emplace_back(sys);
                sys->OnCreate();
            }
        }
    }

    int32_t SystemManager::AddSystem(std::string_view id)
    {
        auto n = FindSystem(id);
        if (n != -1)
            return n;

        auto it = _system_map.find(id);
        if (it == _system_map.end())
            return -1;

        auto* sys   = it->second.create(_scene, &it->second);
        sys->_index = int32_t(_systems.size());
        _systems.emplace_back(sys);
        sys->OnCreate();
        return sys->_index;
    }

    void SystemManager::DeleteSystem(int32_t sid)
    {
        if (size_t(sid) < _systems.size())
        {
            auto* sys = _systems[sid];
            sys->OnDestroy();
            _systems.erase(_systems.begin() + sid);
            delete sys;
            for (size_t n = 0; n < _systems.size(); ++n)
                _systems[n]->_index = (int32_t)n;
        }
    }

    int32_t SystemManager::MoveSystem(int32_t sid, bool up)
    {
        if (up)
        {
            if (size_t(sid + 1) < _systems.size() && size_t(sid) < _systems.size())
            {
                std::swap(_systems[sid], _systems[sid + 1]);
                return sid + 1;
            }
        }
        else
        {
            if (size_t(sid - 1) < _systems.size() && size_t(sid) < _systems.size())
            {
                std::swap(_systems[sid], _systems[sid - 1]);
                return sid - 1;
            }
        }
        for (size_t n = 0; n < _systems.size(); ++n)
            _systems[n]->_index = (int32_t)n;
        return sid;
    }

    System* SystemManager::GetSystem(int32_t n)
    {
        return _systems[n];
    }

    int32_t SystemManager::FindSystem(std::string_view name) const
    {
        for (size_t i = 0; i < _systems.size(); ++i)
        {
            if (_systems[i]->GetName() == name)
            {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    bool SystemManager::IsValid(int32_t sid) const
    {
        return (sid >= 0 && size_t(sid) < _systems.size() && _systems[sid] != nullptr);
    }

    void SystemManager::OnStartRunning()
    {
        std::for_each(_systems.begin(), _systems.end(), [](System* system) { system->OnStartRunning(); });
    }

    void SystemManager::OnStopRunning()
    {
        std::for_each(_systems.begin(), _systems.end(), [](System* system) { system->OnStopRunning(); });
    }

    void SystemManager::Update(float dt)
    {
        std::for_each(_systems.begin(),
                      _systems.end(),
                      [dt](System* system)
                      {
                          if (system->ShouldRunSystem())
                              system->Update(dt);
                      });
    }

    void SystemManager::FixedUpdate(float dt)
    {
        std::for_each(_systems.begin(),
                      _systems.end(),
                      [dt](System* system)
                      {
                          if (system->ShouldRunSystem())
                              system->FixedUpdate(dt);
                      });
    }

    int32_t SystemManager::ImguiMenu()
    {
        int32_t idx = -1;
        for (auto it : _system_map)
        {
            if (ImGui::MenuItem(it.second.label.data()))
            {
                idx = AddSystem(it.first);
            }
        }
        return idx;
    }

    bool SystemManager::ImguiSystems(int32_t* active)
    {
        bool ret{};
        for (size_t i = 0; i < _systems.size(); ++i)
        {
            auto* sys = _systems[i];
            if (ImGui::Selectable(sys->GetSystemInfo()->label.data(), sys->_index == *active))
            {
                *active = sys->_index;
                ret  = true;
            }
        }
        return ret;
    }

} // namespace fin
