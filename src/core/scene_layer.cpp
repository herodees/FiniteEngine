#include "scene_layer.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "application.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{
    void BeginDefaultMenu(const char* id)
    {
        ImGui::LineItem(ImGui::GetID(id), {-1, ImGui::GetFrameHeightWithSpacing()}).Space();
    }

    bool EndDefaultMenu()
    {
        ImGui::Line()
            .Spring()
            .PushStyle(ImStyle_Button, -100, gSettings.visible_grid)
            .Text(ICON_FA_BORDER_ALL)
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -101, gSettings.visible_isometric)
            .Text(ICON_FA_MAP_LOCATION_DOT)
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -102, gSettings.visible_collision)
            .Text(ICON_FA_VECTOR_SQUARE)
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -103, gSettings.visible_navgrid)
            .Text(ICON_FA_MOUNTAIN_SUN)
            .PopStyle();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == -100)
                gSettings.visible_grid = !gSettings.visible_grid;
            if (ImGui::Line().HoverId() == -101)
                gSettings.visible_isometric = !gSettings.visible_isometric;
            if (ImGui::Line().HoverId() == -102)
                gSettings.visible_collision = !gSettings.visible_collision;
            if (ImGui::Line().HoverId() == -103)
                gSettings.visible_navgrid = !gSettings.visible_navgrid;
            return true;
        }
        return false;
    }

    SceneLayer* SceneLayer::Create(msg::Var& ar, Scene* scene)
    {
        if (auto* obj = Create(ar["type"].str()))
        {
            obj->_parent = scene;
            obj->Resize(scene->GetSceneSize());
            obj->Deserialize(ar);
            return obj;
        }
        return nullptr;
    }

    SceneLayer* SceneLayer::Create(std::string_view t)
    {
        if (t == LayerType::Object)
            return CreateObject();
        if (t == LayerType::Sprite)
            return CreateSprite();
        if (t == LayerType::Region)
            return CreateRegion();

        return CreateSprite();
    }

    SceneLayer::SceneLayer(std::string_view t)
    {
        _type = t;
    }

    std::string& SceneLayer::GetName()
    {
        return _name;
    }

    std::string_view& SceneLayer::GetIcon()
    {
        return _icon;
    }

    uint32_t SceneLayer::GetColor() const
    {
        return _color;
    }

    Scene* SceneLayer::GetScene()
    {
        return _parent;
    }

    Vec2f SceneLayer::GetCursorPos() const
    {
        const auto p{::GetMousePosition()};
        return {p.x + _region.x, p.y + _region.y};
    }

    StringView SceneLayer::GetName() const
    {
        return _name;
    }


    void SceneLayer::Serialize(msg::Var& ar)
    {
        ar.set_item("type", _type);
        ar.set_item("name", _name);
        if (IsHidden())
            ar.set_item("hidden", true);
        if (IsDisabled())
            ar.set_item("disabled", true);
    }

    void SceneLayer::Deserialize(msg::Var& ar)
    {
        _name = ar["name"].str();
        Hide(ar.get_item("hidden").get(false));
        Disable(ar.get_item("disabled").get(false));
    }

    void SceneLayer::Resize(Vec2f size)
    {
    }

    void SceneLayer::Clear()
    {
    }

    void SceneLayer::Init()
    {
    }

    void SceneLayer::Deinit()
    {
    }

    void SceneLayer::Activate(const Rectf& region)
    {
        _region = region;
    }

    void SceneLayer::Update(float dt)
    {
    }

    void SceneLayer::FixedUpdate(float dt)
    {
    }

    void SceneLayer::Render(Renderer& dc)
    {
    }

    ObjectLayer* SceneLayer::Objects()
    {
        return nullptr;
    }

    void SceneLayer::ImguiUpdate(bool items)
    {
    }

    void SceneLayer::ImguiSetup()
    {
        ImGui::InputText("Name", &_name);
    }

    void SceneLayer::ImguiWorkspace(ImGui::CanvasParams& canvas)
    {
    }

    void SceneLayer::ImguiWorkspaceMenu()
    {
    }




    LayerManager::~LayerManager()
    {
        Clear();
    }

    int32_t LayerManager::AddLayer(SceneLayer* layer)
    {
        _layers.emplace_back(layer);
        layer->_parent = &_scene;
        layer->_index  = int32_t(_layers.size()) - 1;
        layer->Resize(_scene.GetSceneSize());
        return layer->_index;
    }

    void LayerManager::RemoveLayer(int32_t n)
    {
        delete _layers[n];
        _layers.erase(_layers.begin() + n);
        for (; n < _layers.size(); ++n)
            _layers[n]->_index = (int32_t)n;
    }

    int32_t LayerManager::MoveLayer(int32_t layer, bool up)
    {
        if (up)
        {
            if (size_t(layer + 1) < _layers.size() && size_t(layer) < _layers.size())
            {
                std::swap(_layers[layer], _layers[layer + 1]);
                return layer + 1;
            }
        }
        else
        {
            if (size_t(layer - 1) < _layers.size() && size_t(layer) < _layers.size())
            {
                std::swap(_layers[layer], _layers[layer - 1]);
                return layer - 1;
            }
        }
        for (size_t n = 0; n < _layers.size(); ++n)
            _layers[n]->_index = (int32_t)n;
        return layer;
    }

    SceneLayer* LayerManager::GetActiveLayer()
    {
        if (size_t(_active_layer) < _layers.size())
            return _layers[_active_layer];
        return nullptr;
    }

    void LayerManager::SetActiveLayer(SceneLayer* layer)
    {
        if (!layer)
            _active_layer = -1;
        else
            _active_layer = layer->_index;
    }

    SceneLayer* LayerManager::FindLayer(std::string_view name) const
    {
        for (auto* ly : _layers)
        {
            if (ly->GetName() == name)
                return ly;
        }
        return nullptr;
    }

    void LayerManager::SetSize(Vec2f size)
    {
        for (auto* ly : _layers)
        {
            ly->Resize(size);
        }
    }

    void LayerManager::Activate(const Rectf& region)
    {
        for (auto* ly : _layers)
        {
            ly->Activate(region);
        }
    }

    void LayerManager::Render(Renderer& dc)
    {
        for (auto* el : _layers)
        {
            el->Render(dc);
        }
    }

    void LayerManager::Update(float dt)
    {
        std::for_each(_layers.begin(), _layers.end(), [dt](auto* lyr) { lyr->Update(dt); });
    }

    void LayerManager::FixedUpdate(float dt)
    {
        std::for_each(_layers.begin(), _layers.end(), [dt](auto* lyr) { lyr->FixedUpdate(dt); });
    }

    void LayerManager::Init()
    {
        std::for_each(_layers.begin(), _layers.end(), [](auto* lyr) { lyr->Init(); });
    }

    void LayerManager::Deinit()
    {
        std::for_each(_layers.rbegin(), _layers.rend(), [](auto* lyr) { lyr->Deinit(); });
    }

    void LayerManager::Clear()
    {
        std::for_each(_layers.begin(), _layers.end(), [](auto* p) { delete p; });
        _layers.clear();
    }

    void LayerManager::Serialize(msg::Var& ar)
    {
        for (auto lyr : _layers)
        {
            msg::Var layer;
            lyr->Serialize(layer);
            ar.push_back(layer);
        }
    }

    void LayerManager::Deserialize(msg::Var& ar)
    {
        for (auto ly : ar.elements())
        {
            AddLayer(SceneLayer::Create(ly, &_scene));
        }
    }

    bool LayerManager::ImguiLayers(int32_t* active)
    {
        if (!active)
            active = &_active_layer;

        bool ret = false;
        if (ImGui::BeginChildFrame(ImGui::GetID("lyrssl"), {-1, 100}, 0))
        {
            int n = 0;
            for (auto* ly : _layers)
            {
                if (ImGui::LineSelect(ImGui::GetID(ly), *active == n)
                        .Space()
                        .PushStyle(ImStyle_Button, 1)
                        .Text(ly->IsHidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE)
                        .PopStyle()
                        .Space()
                        .PushColor(ly->GetColor())
                        .Text(ly->GetIcon())
                        .Space()
                        .Text(ly->GetName())
                        .Spring()
                        .PopColor()
                        .Text(_active_layer == n ? ICON_FA_BRUSH : "")
                        .End())
                {
                    ret = true;
                    *active = n;
                    if (ImGui::Line().HoverId() == 1)
                    {
                        ly->Hide(!ly->IsHidden());
                    }
                }
                ++n;
            }
        }

        ImGui::EndChildFrame();
        return ret;
    }

    void LayerManager::ImguiSetup()
    {
        if (ImGui::LineItem(ImGui::GetID("mnu"), {-1, ImGui::GetFrameHeight()})
                .PushStyle(ImStyle_Button, 1)
                .Text(ICON_FA_LAPTOP " Add ")
                .PopStyle()
                .PushStyle(ImStyle_Button, 2)
                .Text(ICON_FA_ARROW_UP)
                .PopStyle()
                .PushStyle(ImStyle_Button, 3)
                .Text(ICON_FA_ARROW_DOWN)
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 4)
                .Text(ICON_FA_BAN)
                .PopStyle()
                .End())
        {
            if (ImGui::Line().HoverId() == 1)
            {
                ImGui::OpenPopup("LayerMenu");
            }

            if (ImGui::Line().HoverId() == 2)
            {
                _active_layer = MoveLayer(_active_layer, false);
            }
            if (ImGui::Line().HoverId() == 3)
            {
                _active_layer = MoveLayer(_active_layer, true);
            }
            if (ImGui::Line().HoverId() == 4 && GetActiveLayer())
            {
                RemoveLayer(_active_layer);
            }
        }

        if (ImGui::BeginPopup("LayerMenu"))
        {
            if (ImGui::MenuItem(ICON_FA_IMAGE " Sprite layer"))
                _active_layer = AddLayer(SceneLayer::Create(LayerType::Sprite));
            if (ImGui::MenuItem(ICON_FA_MAP_LOCATION_DOT " Region layer"))
                _active_layer = AddLayer(SceneLayer::Create(LayerType::Region));
            if (ImGui::MenuItem(ICON_FA_BOX " Object layer"))
                _active_layer = AddLayer(SceneLayer::Create(LayerType::Object));
            ImGui::EndPopup();
        }

        if (ImGui::BeginChildFrame(ImGui::GetID("lyrs"), {-1, -1}))
        {
            int n = 0;
            for (auto* ly : _layers)
            {
                if (ImGui::LineSelect(ImGui::GetID(ly), _active_layer == n)
                        .PushColor(ly->GetColor())
                        .Text(ly->GetIcon())
                        .Space()
                        .Text(ly->GetName())
                        .End())
                {
                    _active_layer = n;
                }
                ++n;
            }
        }
        ImGui::EndChildFrame();
    }

    bool LayerManager::ImguiScene()
    {
        if (ImGui::BeginChild(ImGui::GetID("lyrssl"), {-1, 100}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY))
        {
            int n = 0;
            for (auto* ly : _layers)
            {
                if (ImGui::LineSelect(ImGui::GetID(ly), _active_layer == n)
                        .Space()
                        .PushStyle(ImStyle_Button, 1)
                        .Text(ly->IsHidden() ? ICON_FA_EYE_SLASH : ICON_FA_EYE)
                        .PopStyle()
                        .Space()
                        .PushColor(ly->GetColor())
                        .Text(ly->GetIcon())
                        .Space()
                        .Text(ly->GetName())
                        .Spring()
                        .PopColor()
                        .Text(_active_layer == n ? ICON_FA_CIRCLE_DOT : "")
                        .Space()
                        .End())
                {
                    _active_layer = n;
                    if (ImGui::Line().HoverId() == 1)
                    {
                        ly->Hide(!ly->IsHidden());
                    }
                }
                ++n;
            }
        }
        ImGui::EndChild();

        if (auto* lyr = GetActiveLayer())
        {
            ImGui::PushID("lyit");
            lyr->ImguiUpdate(true);
            ImGui::PopID();
        }

        return false;
    }

    std::span<SceneLayer*> LayerManager::GetLayers()
    {
        return std::span<SceneLayer*>();
    }




} // namespace fin
