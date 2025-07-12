#pragma once

#include "utils/lib_utils.hpp"
#include "ecs/factory.hpp"
#include "ecs/system.hpp"
#include "include.hpp"
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

    class Scene : private SystemManager, private ComponentFactory, private LayerManager
    {
        friend class SpriteSceneObject;
        using Plugins = std::vector<std::pair<DynamicLibrary, IGamePlugin*>>;
    public:
        Scene();
        ~Scene();

        const std::string& GetPath() const;
        void               Init(std::string_view root);
        void               SetSize(Vec2f size);
        void               SetMode(SceneMode sm);
        SceneMode          GetMode() const;
        void               ActivateEditor();
        void               ActicateGrid(const Recti& screen);
        Vec2i              GetActiveSize() const;
        Vec2i              GetSceneSize() const;
        void               SetCameraPosition(Vec2f pos, float speed = 1.f);
        void               SetCameraCenter(Vec2f pos, float speed = 1.f);
        Vec2f              GetCameraCenter() const;
        const Camera&      GetCamera() const;
        Color              GetBackgroundColor() const;
        void               SetBackgroundColor(Color clr);
        IGamePlugin*       GetPlugin(std::string_view guid) const;
        std::string&       GetTag(int32_t index);
        int32_t            GetTag(std::string_view tag) const;
        std::string        GetTagName(uint32_t tag) const;

        void Init();
        void Deinit();
        void Render(Renderer& dc);
        void Update(float dt);
        void PostUpdate(float dt);
        void FixedUpdate(float dt);
        void Clear();
        void Serialize(msg::Var& ar);
        void Deserialize(msg::Var& ar);
        void Load(std::string_view path);
        void Save(std::string_view path, bool change_path = true);

        RenderTexture2D&  GetCanvas();
        ComponentFactory& GetFactory();
        SystemManager&    GetSystems();
        LayerManager&     GetLayers();

        void ImguiWorkspace();
        void ImguiProps();
        void ImguiItems();
        void ImguiMenu();
        void ImguiShowProperties(bool show);
        bool ImguiTagInput(const char* label, uint32_t& tag) const;

        bool UnloadPlugin(IGamePlugin* plug);
        bool LoadPlugin(const std::string& dir, const std::string& plugin);

    private:

        Plugins                  _plugins;
        bool                     _show_properties{};
        bool                     _edit_region{};
        bool                     _edit_prefabs{};
        bool                     _was_inited{};
        SceneMode                _mode{SceneMode::Play};
        Vec2f                    _goto;
        float                    _goto_speed{};
        Camera                   _camera;
        RenderTexture2D          _canvas;
        std::string              _path;
        Vec2f                    _size{1024, 1024};
        Color                    _background{BLACK};
        std::array<std::string, 32> _tags;
    };


} // namespace fin
