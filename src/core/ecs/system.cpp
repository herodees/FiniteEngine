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

    int32_t System::get_index() const
    {
        return _index;
    }

    bool System::should_run_system() const
    {
        return !(_flags & SystemFlags_Disabled);
    }

} // namespace fin
