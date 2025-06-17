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



    namespace LayerType
    {
        constexpr std::string_view Object = "obj";
        constexpr std::string_view Sprite = "spr";
        constexpr std::string_view Region = "reg";
    } // namespace LayerType

    class SceneLayer : public Layer
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

        std::string&      GetName();
        std::string_view& GetIcon();
        uint32_t          GetColor() const;
        Scene*            GetScene();

        void Serialize(msg::Var& ar) override;
        void Deserialize(msg::Var& ar) override;
        void Resize(Vec2f size) override;
        void Clear() override;
        void Init() override;
        void Deinit() override;
        void Activate(const Rectf& region) override;
        void Update(float dt) override;
        void FixedUpdate(float dt) override;
        void Render(Renderer& dc) override;
        ObjectLayer* Objects() override;

        Vec2f            GetCursorPos() const final;
        StringView       GetName() const final;

        virtual bool ImguiUpdate(bool items);
        virtual void ImguiSetup();
        virtual bool ImguiWorkspace(ImGui::CanvasParams& canvas);
        virtual bool ImguiWorkspaceMenu(ImGui::CanvasParams& canvas);

    protected:
        Scene*           _parent{};
        std::string      _name;
        std::string_view _icon;
        uint32_t         _color{0xffffffff};
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
        void        SetActiveLayer(SceneLayer* layer);
        SceneLayer* FindLayer(std::string_view name) const;

        void SetSize(Vec2f size);
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


    void BeginDefaultMenu(const char* id, ImGui::CanvasParams& canvas);
    bool EndDefaultMenu(ImGui::CanvasParams& canvas);

} // namespace fin
