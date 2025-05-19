#include "scene_layer.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "application.hpp"
#include "scene_object.hpp"
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
            .PushStyle(ImStyle_Button, -100, g_settings.visible_grid)
            .Text(ICON_FA_BORDER_ALL )
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -101, g_settings.visible_isometric)
            .Text(ICON_FA_MAP_LOCATION_DOT )
            .PopStyle()
            .Space()
            .PushStyle(ImStyle_Button, -102, g_settings.visible_collision)
            .Text(ICON_FA_VECTOR_SQUARE)
            .PopStyle();

        if (ImGui::Line().End())
        {
            if (ImGui::Line().HoverId() == -100)
                g_settings.visible_grid = !g_settings.visible_grid;
            if (ImGui::Line().HoverId() == -101)
                g_settings.visible_isometric = !g_settings.visible_isometric;
            if (ImGui::Line().HoverId() == -102)
                g_settings.visible_collision = !g_settings.visible_collision;
            return true;
        }
        return false;
    }

    SceneLayer* SceneLayer::create(msg::Value& ar, Scene* scene)
    {
        if (auto* obj = create(Type(ar["type"].get(0))))
        {
            obj->_parent = scene;
            obj->deserialize(ar);
            return obj;
        }
        return nullptr;
    }

    SceneLayer* SceneLayer::create(Type t)
    {
        switch (t)
        {
            case Type::Sprite:
                return create_sprite();
            case Type::Region:
                return create_region();
            case Type::Isometric:
                return create_isometric();
        }
        return nullptr;
    }

    SceneLayer::SceneLayer(Type t) : _type(t)
    {
    }

    SceneLayer::Type SceneLayer::type() const
    {
        return _type;
    }

    std::string& SceneLayer::name()
    {
        return _name;
    }

    std::string_view& SceneLayer::icon()
    {
        return _icon;
    }

    uint32_t SceneLayer::color() const
    {
        return _color;
    }

    Scene* SceneLayer::parent()
    {
        return _parent;
    }

    const Rectf& SceneLayer::region() const
    {
        return _region;
    }

    Vec2f SceneLayer::screen_to_view(Vec2f pos) const
    {
        return Vec2f(pos.x + _region.x, pos.y + _region.y); 
    }

    Vec2f SceneLayer::view_to_screen(Vec2f pos) const
    {
        return Vec2f(pos.x - _region.x, pos.y - _region.y); 
    }

    Vec2f SceneLayer::get_mouse_position() const
    {
        const auto p{GetMousePosition()};
        return {p.x + _region.x, p.y + _region.y};
    }

    void SceneLayer::serialize(msg::Writer& ar)
    {
        ar.member("type", (int)_type);
        ar.member("name", _name);
    }

    void SceneLayer::deserialize(msg::Value& ar)
    {
        _type = Type(ar["type"].get(0));
        _name = ar["name"].str();
    }

    void SceneLayer::resize(Vec2f size)
    {
    }

    void SceneLayer::clear()
    {
    }

    void SceneLayer::init()
    {
    }

    void SceneLayer::deinit()
    {
    }

    void SceneLayer::activate(const Rectf& region)
    {
        _region = region;
    }

    void SceneLayer::update(float dt)
    {
    }

    void SceneLayer::render(Renderer& dc)
    {
    }

    void SceneLayer::render_edit(Renderer& dc)
    {
    }

    std::span<const Vec2i> SceneLayer::find_path(const IsoSceneObject* obj, Vec2i target)
    {
        return std::span<const Vec2i>();
    }

    void SceneLayer::moveto(IsoSceneObject* obj, Vec2f pos)
    {
    }

    void SceneLayer::imgui_update(bool items)
    {
    }

    void SceneLayer::imgui_setup()
    {
        ImGui::InputText("Name", &_name);
    }

    void SceneLayer::imgui_workspace(Params& params, DragData& drag)
    {
    }

    void SceneLayer::imgui_workspace_menu()
    {
    }

    bool SceneLayer::is_hidden() const
    {
        return _hidden;
    }

    bool SceneLayer::is_active() const
    {
        return _active;
    }

    void SceneLayer::hide(bool b)
    {
        _hidden = b;
    }

    void SceneLayer::activate(bool a)
    {
        _active = a;
    }



} // namespace fin
