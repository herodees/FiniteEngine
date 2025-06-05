#pragma once

#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include "ecs/behavior.hpp"
#include "include.hpp"
#include "path_finder.h"
#include "renderer.hpp"
#include "scene_layer.hpp"
#include "shared_resource.hpp"

namespace fin
{
    constexpr int32_t SCENE_VERSION = 1;

    struct Camera
    {
        Vec2f position;
        Vec2i size;
        float zoom{1.f};
    };

    enum class SceneMode
    {
        Play,
        Scene,
        Prefab,
    };

    class Scene : private ScriptFactory, private SystemManager, private ComponentFactory, private LayerManager
    {
        friend class SpriteSceneObject;

    public:
        Scene();
        ~Scene();

        const std::string& GetPath() const;
        void               Init(std::string_view root);
        void               SetSize(Vec2f size);
        void               SetMode(SceneMode sm);
        SceneMode          GetMode() const;
        void               ActicateGrid(const Recti& screen);
        Vec2i              GetActiveSize() const;
        Vec2i              GetSceneSize() const;
        void               SetCameraPosition(Vec2f pos, float speed = 1.f);
        void               SetCameraCenter(Vec2f pos, float speed = 1.f);
        Vec2f              GetCameraCenter() const;
        const Camera&      GetCamera() const;
        Color              GetBackgroundColor() const;
        void               SetBackgroundColor(Color clr);

        void Init();
        void Deinit();
        void Render(Renderer& dc);
        void Update(float dt);
        void FixedUpdate(float dt);
        void Clear();
        void Serialize(msg::Var& ar);
        void Deserialize(msg::Var& ar);
        void Load(std::string_view path);
        void Save(std::string_view path, bool change_path = true);

        RenderTexture2D&  GetCanvas();
        ComponentFactory& GetFactory();
        SystemManager&    GetSystems();
        ScriptFactory&    GetScripts();
        LayerManager&     GetLayers();

        void ImguiWorkspace();
        void ImguiProps();
        void ImguiItems();
        void ImguiMenu();
        void ImguiShowProperties(bool show);

    private:

        bool                     _show_properties{};
        bool                     _edit_region{};
        bool                     _edit_prefabs{};
        SceneMode                _mode{SceneMode::Play};
        Vec2f                    _goto;
        float                    _goto_speed{};
        Camera                   _camera;
        RenderTexture2D          _canvas;
        std::string              _path;
        Vec2f                    _size{1024, 1024};
        Color                    _background{BLACK};
    };


} // namespace fin
