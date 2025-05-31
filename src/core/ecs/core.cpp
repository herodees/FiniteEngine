#include "core.hpp"
#include "base.hpp"
#include "core/scene.hpp"

namespace fin::ecs
{
    void register_core_systems(SystemManager& fact)
    {
        fact.register_system<Navigation, "navs", "Navigation">(SystemFlags_Default);
        fact.register_system<Behavior, "bhvs", "Behavior">(SystemFlags_Default);
    }




    Navigation::Navigation(Scene& s) : System(s)
    {
    }

    void Navigation::update(float dt)
    {
        auto layers = scene().layers();
        for (auto* layer : layers)
        {
            if (!layer->is_active())
                continue;

            if (layer->type() != LayerType::Object)
                continue;

            update_objects(dt, static_cast<ObjectSceneLayer*>(layer));
        }
    }

    inline void update_path(ecs::Base* base, ecs::Body* body, ecs::Path* path, Navmesh& navmesh)
    {
        if (path->_path.empty())
        {
            body->_speed = {}; // stop
            return;
        }

        Vec2f pos         = base->_position;
        Vec2i target_cell = path->_path.front();
        Vec2f target_pos  = navmesh.cellToWorld(target_cell);

        Vec2f delta = target_pos - pos;
        float dist  = delta.length();

        const float threshold = 1.0f; // or half a cell

        if (dist < threshold)
        {
            // Reached current target
            path->_path.erase(path->_path.begin());

            if (path->_path.empty())
            {
                body->_speed = {};
                return;
            }

            // Update to next target
            target_cell = path->_path.front();
            target_pos  = navmesh.cellToWorld(target_cell);
            delta       = target_pos - pos;
        }

        Vec2f dir        = delta.normalized();
        float move_speed = 60.0f; // or body->max_speed
        body->_speed     = dir * move_speed;
    }

    void Navigation::update_objects(float dt, ObjectSceneLayer* layer)
    {
        auto&      factory  = scene().factory();
        auto&      registry = factory.get_registry();
        const auto view     = registry.view<ecs::Base, ecs::Body>();
        auto&      navmesh  = layer->get_navmesh();

        for (size_t n = 0; n < layer->get_active_count(); ++n)
        {
            auto ent = layer->get_active(n);

            if (!registry.valid(ent) || !view.contains(ent))
                continue;

            Base* base = ecs::Base::get(ent);
            Body* body = ecs::Body::get(ent);

            if (ecs::Path::contains(ent))
            {
                update_path(base, body, ecs::Path::get(ent), navmesh);
            }

            if (body->_speed == Vec2f())
            {
                continue;
            }

            body->_previous_position = base->_position;

            Vec2f from = base->_position;
            Vec2f to   = from + body->_speed * dt;

            Vec2i grid_from = navmesh.worldToCell(from);
            Vec2i grid_to   = navmesh.worldToCell(to);

            Vec2f result = from;

            int dx = grid_to.x - grid_from.x;
            int dy = grid_to.y - grid_from.y;

            int steps = std::max(std::abs(dx), std::abs(dy));
            if (steps == 0)
            {
                // Same cell, no need to trace
                if (navmesh.isWalkable(grid_to.x, grid_to.y))
                    result = to;
            }
            else
            {
                Vec2f step  = (to - from) / float(steps);
                Vec2f probe = from;

                for (int i = 0; i < steps; ++i)
                {
                    probe += step;
                    Vec2i cell = navmesh.worldToCell(probe);

                    if (!navmesh.isWalkable(cell.x, cell.y))
                        break;

                    result = probe;
                }
            }
            base->_position = result;
        }
    }

    bool Navigation::imgui_setup()
    {
        return false;
    }



    Behavior::Behavior(Scene& s) : System(s)
    {
    }

    void Behavior::update(float dt)
    {
        auto layers = scene().layers();
        for (auto* layer : layers)
        {
            if (!layer->is_active())
                continue;
            if (layer->type() != LayerType::Object)
                continue;

            update_objects(dt,static_cast<ObjectSceneLayer*>(layer));
        }
    }

    void Behavior::update_objects(float dt, ObjectSceneLayer* layer)
    {
        for (size_t n = 0; n < layer->get_active_count(); ++n)
        {
            auto ent = layer->get_active(n);

            if (ecs::Script::contains(ent))
            {
                auto* scr = ecs::Script::get(ent);
                auto* s = scr->_script;
                while (s)
                {
                    s->OnUpdate(ent, dt);
                    s = s->next;
                }
            }
        }
    }

    bool Behavior::imgui_setup()
    {
        return false;
    }

} // namespace fin::ecs
