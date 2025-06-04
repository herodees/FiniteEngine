#pragma once

#include "include.hpp"
#include "utils/lquery.hpp"
#include "navmesh.hpp"

namespace ImGui
{
    struct CanvasParams;
}

namespace fin
{
    class Renderer;
    class Scene;
    class IsoSceneObject;
    struct Params;
    struct DragData;

    constexpr int32_t tile_size(512);

    namespace LayerType
    {
        constexpr std::string_view Object = "obj";
        constexpr std::string_view Sprite = "spr";
        constexpr std::string_view Region = "reg";
    } // namespace LayerType

    class SceneLayer
    {
        friend class Scene;
    public:
        static SceneLayer* Create(msg::Var& ar, Scene* scene);
        static SceneLayer* Create(std::string_view t);

        static SceneLayer* CreateSprite();
        static SceneLayer* CreateRegion();
        static SceneLayer* CreateObject();

        SceneLayer(std::string_view t);
        virtual ~SceneLayer() = default;

        std::string_view  GetType() const;
        std::string&      GetName();
        std::string_view& GetIcon();
        uint32_t          GetColor() const;
        int32_t           index() const;
        Scene*            parent();
        const Rectf&      region() const;
        Vec2f             ScreenToView(Vec2f pos) const;
        Vec2f             ViewToScreen(Vec2f pos) const;
        Vec2f             GetMousePosition() const;

        virtual void Serialize(msg::Var& ar);
        virtual void Deserialize(msg::Var& ar);
        virtual void Resize(Vec2f size);
        virtual void Clear();
        virtual void Init();
        virtual void Deinit();
        virtual void Activate(const Rectf& region);
        virtual void Update(float dt);
        virtual void FixedUpdate(float dt);
        virtual void Render(Renderer& dc);

        virtual void ImguiUpdate(bool items);
        virtual void ImguiSetup();
        virtual void ImguiWorkspace(ImGui::CanvasParams& canvas);
        virtual void ImguiWorkspaceMenu();

        bool IsHidden() const;
        bool IsActive() const;

        void Hide(bool b);
        void Activate(bool a);

    protected:
        Scene*           _parent{};
        int32_t          _index{-1};
        std::string      _name;
        std::string      _type;
        std::string_view _icon;
        uint32_t         _color{0xffffffff};
        Rectf            _region;
        bool             _hidden{};
        bool             _active{};
        friend class LayerManager;
    };



    class LayerManager
    {
    public:
        LayerManager(Scene& scene) : _scene(scene) {};
        ~LayerManager();

        int32_t     AddLayer(SceneLayer* layer);
        void        RemoveLayer(int32_t n);
        int32_t     MoveLayer(int32_t layer, bool up);
        SceneLayer* GetActiveLayer();
        SceneLayer* FindLayer(std::string_view name) const;

        void set_size(Vec2f size);
        void Activate(const Rectf& region);
        void Render(Renderer& dc);
        void Update(float dt);
        void FixedUpdate(float dt);
        void Init();
        void Deinit();
        void Clear();
        void Serialize(msg::Var& ar);
        void Deserialize(msg::Var& ar);

        bool ImguiLayers(int32_t* active);
        void ImguiSetup();
        bool ImguiScene();

        std::span<SceneLayer*> GetLayers();

    private:
        Scene&                   _scene;
        std::vector<SceneLayer*> _layers;
        int32_t                  _active_layer{0};
    };


    void BeginDefaultMenu(const char* id);
    bool EndDefaultMenu();

} // namespace fin
