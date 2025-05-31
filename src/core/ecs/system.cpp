#include "system.hpp"
#include "core/scene.hpp"

namespace fin
{
    System::System(Scene& s) : _scene(s)
    {
    }

    Registry& System::registry()
    {
        return _scene.factory().get_registry();
    }

    Scene& System::scene()
    {
        return _scene;
    }

    std::string_view System::name() const
    {
        return _info->name;
    }

    SystemInfo* System::info() const
    {
        return _info;
    }

    bool System::should_run_system() const
    {
        return !(_flags & SystemFlags_Disabled);
    }

    bool System::imgui_setup()
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

    void SystemManager::add_defaults()
    {
        for (auto& it : _system_map)
        {
            if (!(it.second.flags & SystemFlags_Extra))
            {
                auto* sys = it.second.create(_scene, &it.second);
                sys->_index = int32_t(_systems.size());
                _systems.emplace_back(sys);
                sys->on_create();
            }
        }
    }

    int32_t SystemManager::add_system(std::string_view id)
    {
        auto n = find_system(id);
        if (n != -1)
            return n;

        auto it = _system_map.find(id);
        if (it == _system_map.end())
            return -1;

        auto* sys   = it->second.create(_scene, &it->second);
        sys->_index = int32_t(_systems.size());
        _systems.emplace_back(sys);
        sys->on_create();
        return sys->_index;
    }

    void SystemManager::delete_system(int32_t sid)
    {
        if (size_t(sid) < _systems.size())
        {
            auto* sys = _systems[sid];
            sys->on_destroy();
            _systems.erase(_systems.begin() + sid);
            delete sys;
            for (size_t n = 0; n < _systems.size(); ++n)
                _systems[n]->_index = (int32_t)n;
        }
    }

    int32_t SystemManager::move_system(int32_t sid, bool up)
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

    System* SystemManager::get_system(int32_t n)
    {
        return _systems[n];
    }

    int32_t SystemManager::find_system(std::string_view name) const
    {
        for (size_t i = 0; i < _systems.size(); ++i)
        {
            if (_systems[i]->name() == name)
            {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    bool SystemManager::is_valid(int32_t sid) const
    {
        return (sid >= 0 && size_t(sid) < _systems.size() && _systems[sid] != nullptr);
    }

    void SystemManager::on_start_runing()
    {
        std::for_each(_systems.begin(), _systems.end(), [](System* system) { system->on_start_runing(); });
    }

    void SystemManager::on_stop_runing()
    {
        std::for_each(_systems.begin(), _systems.end(), [](System* system) { system->on_stop_runing(); });
    }

    void SystemManager::update(float dt)
    {
        std::for_each(_systems.begin(),
                      _systems.end(),
                      [dt](System* system)
                      {
                          if (system->should_run_system())
                              system->update(dt);
                      });
    }

    void SystemManager::fixed_update(float dt)
    {
        std::for_each(_systems.begin(),
                      _systems.end(),
                      [dt](System* system)
                      {
                          if (system->should_run_system())
                              system->fixed_update(dt);
                      });
    }

    int32_t SystemManager::imgui_menu()
    {
        int32_t idx = -1;
        for (auto it : _system_map)
        {
            if (ImGui::MenuItem(it.second.label.data()))
            {
                idx = add_system(it.first);
            }
        }
        return idx;
    }

    bool SystemManager::imgui_systems(int32_t* active)
    {
        bool ret{};
        for (size_t i = 0; i < _systems.size(); ++i)
        {
            auto* sys = _systems[i];
            if (ImGui::Selectable(sys->info()->label.data(), sys->_index == *active))
            {
                *active = sys->_index;
                ret  = true;
            }
        }
        return ret;
    }

} // namespace fin
