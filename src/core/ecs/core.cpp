#include "core.hpp"
#include "builtin.hpp"
#include "core/scene.hpp"
#include "core/scene_layer_object.hpp"


namespace fin::ecs
{
    void RegisterCoreSystems(SystemManager& fact)
    {
        fact.RegisterSystem<Navigation, "navs", "Navigation">(SystemFlags_Default);
        fact.RegisterSystem<CameraController, "cmrs", "Camera Controller">(SystemFlags_Default);
    }




    Navigation::Navigation(Scene& s) : System(s)
    {
    }

    void Navigation::Update(float dt)
    {
        auto layers = GetScene().GetLayers().GetLayers();
        for (auto* layer : layers)
        {
            if (layer->IsDisabled())
                continue;
            if (layer->GetType() != LayerType::Object)
                continue;

            update_objects(dt, static_cast<ObjectSceneLayer*>(layer));
        }
    }

    inline void update_path(CBase& base, CBody& body, CPath& path, Navmesh& navmesh)
    {
        if (path._path.empty())
        {
            body._speed = {}; // stop
            return;
        }

        Vec2f pos         = base._position;
        Vec2i target_cell = path._path.front();
        Vec2f target_pos  = navmesh.cellToWorld(target_cell);

        Vec2f delta = target_pos - pos;
        float dist  = delta.length();

        const float threshold = 1.0f; // or half a cell

        if (dist < threshold)
        {
            // Reached current target
            path._path.erase(path._path.begin());

            if (path._path.empty())
            {
                body._speed = {};
                return;
            }

            // Update to next target
            target_cell = path._path.front();
            target_pos  = navmesh.cellToWorld(target_cell);
            delta       = target_pos - pos;
        }

        Vec2f dir        = delta.normalized();
        float move_speed = 60.0f; // or body->max_speed
        body._speed     = dir * move_speed;
    }

    void Navigation::update_objects(float dt, ObjectSceneLayer* layer)
    {
        auto&      factory  = GetScene().GetFactory();
        auto&      registry = factory.GetRegister();
        const auto view     = View<CBase, CBody>();
        auto&      navmesh  = layer->GetNavmesh();
        auto&      objects  = layer->GetObjects(true);

        for (auto ent : objects)
        {
            if (!registry.Valid(ent) || !view.Contains(ent))
                continue;

            CBase& base = Get<CBase>(ent);
            CBody& body = Get<CBody>(ent);

            if (Contains<CPath>(ent))
            {
                update_path(base, body, Get<CPath>(ent), navmesh);
            }

            if (body._speed == Vec2f())
            {
                continue;
            }

            body._previous_position = base._position;

            Vec2f from = base._position;
            Vec2f to   = from + body._speed * dt;

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
            base._position = result;
        }
    }

    bool Navigation::ImguiSetup()
    {
        return false;
    }



    CameraController::CameraController(Scene& s) : System(s)
    {
    }

    void CameraController::SetCamera(Entity cam)
    {
        m_Camera = cam;
    }

    void CameraController::Update(float dt)
    {
        auto& reg = GetRegister();

        if (!reg.Valid(m_Camera) || !Contains<CCamera>(m_Camera) || !Contains<CBase>(m_Camera))
            return;

        auto& camera    = Get<CCamera>(m_Camera);
        auto& transform = Get<CBase>(m_Camera);

      //  Zoom control (mouse wheel or whatever)
      //  float scrollDelta = Input::GetScrollDelta();
      //  camera._zoom -= scrollDelta * camera._zoomSpeed;
      //  camera._zoom = std::clamp(camera._zoom, 0.1f, 10.0f);

        if (reg.Valid(camera._target))
        {
            if (Contains<CBase>(camera._target))
            {
                auto& targetTransform = Get<CBase>(camera._target);
                transform._position   = lerp(transform._position, targetTransform._position, dt * camera._smoothFactor);
                transform.UpdateSparseGrid();
            }
        }
        else
        {
            /*
            // No target: manual camera movement example (WASD)
            Vec2f moveDir{0.f, 0.f};
            if (Input::IsKeyDown(KEY_W))
                moveDir.y += 1.f;
            if (Input::IsKeyDown(KEY_S))
                moveDir.y -= 1.f;
            if (Input::IsKeyDown(KEY_A))
                moveDir.x -= 1.f;
            if (Input::IsKeyDown(KEY_D))
                moveDir.x += 1.f;

            if (moveDir != Vec2f{0.f, 0.f})
            {
                moveDir = glm::normalize(moveDir);
                transform.position += moveDir * camera._moveSpeed * dt;
            }
            */
        }
    }

    void CameraController::OnStartRunning()
    {
        if (m_Camera == entt::null)
        {
            if (!ComponentTraits<CCamera>::set->empty())
           {
                m_Camera = *ComponentTraits<CCamera>::set->begin();
           }
        }
    }

    bool CameraController::ImguiSetup()
    {
        return true;
    }

} // namespace fin::ecs
