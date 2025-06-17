#include "application.hpp"
#include "editor/imgui_control.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "scene_layer.hpp"
#include "utils/imguiline.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class SpriteSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            Atlas::Pack _sprite;
            Rectf       _bbox;
            uint32_t    _index{};

            bool operator==(const Node& ot) const
            {
                return this == &ot;
            }
        };

        SpriteSceneLayer() : SceneLayer(LayerType::Sprite), _spatial({})
        {
            GetName() = "SpriteLayer";
            GetIcon() = ICON_FA_IMAGE;
            _color = 0xffffa0b0;
        };

        void Destroy(int32_t n)
        {
            _spatial.remove(_spatial[n]);
        }

        void Resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void Activate(const Rectf& region) override
        {
            SceneLayer::Activate(region);
            _spatial.activate(region);
            if (_sort_y)
                _spatial.sort_active([&](int a, int b) { return _spatial[a]._bbox.y < _spatial[b]._bbox.y; });
            else
                _spatial.sort_active([&](int a, int b) { return _spatial[a]._index < _spatial[b]._index; });
        }

        void MoveTo(int obj, Vec2f pos)
        {
            auto o = _spatial[obj];
            _spatial.remove(_spatial[obj]);
            o._bbox.x = pos.x;
            o._bbox.y = pos.y;
            _spatial.insert(o);
        }

        void Update(float dt) override
        {
        }

        void Render(Renderer& dc) override
        {
            if (IsHidden())
                return;

            dc.set_color(WHITE);
            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];
                dc.render_texture(nde._sprite.sprite->_texture, nde._sprite.sprite->_source, nde._bbox);
            }

            dc.set_color(WHITE);
            if (_edit._sprite.sprite)
            {
                dc.render_texture(_edit._sprite.sprite->_texture, _edit._sprite.sprite->_source, _edit._bbox);
            }
        }

        void Serialize(msg::Var& ar) override
        {
            SceneLayer::Serialize(ar);

            msg::Var   items;
            items.make_array(_spatial.size());
            auto     Save = [&items](Node& node)
            {
                msg::Var item;
                item.set_item("i", node._index);
                item.set_item("a", node._sprite.atlas->get_path());
                item.set_item("s", node._sprite.sprite->_name);
                item.set_item("x", node._bbox.x);
                item.set_item("y", node._bbox.y);
                items.push_back(item);
            };

            _spatial.for_each(Save);
            ar.set_item("items", items);
            ar.set_item("max_index", _max_index);
            ar.set_item("sort_y", _sort_y);
        }

        void Deserialize(msg::Var& ar) override
        {
            SceneLayer::Deserialize(ar);
            _spatial.clear();
            _spatial.resize(Rectf(0, 0, _parent->GetSceneSize().x, _parent->GetSceneSize().y));

            _max_index = ar["max_index"].get(0u);
            _sort_y    = ar["sort_y"].get(false);
            auto els   = ar["items"];
            for (auto& el : els.elements())
            {
                Node nde;
                nde._sprite = Atlas::load_shared(el["a"].str(), el["s"].str());
                if (nde._sprite.sprite)
                {
                    nde._index       = el["i"].get(0u);
                    nde._bbox.x      = el["x"].get(0.f);
                    nde._bbox.y      = el["y"].get(0.f);
                    nde._bbox.width  = nde._sprite.sprite->_source.width;
                    nde._bbox.height = nde._sprite.sprite->_source.height;
                    _spatial.insert(nde);
                }
            }
        }

        int FindAt(Vec2f position)
        {
            return _spatial.find_at(position.x, position.y);
        }

        int FindActiveAt(Vec2f position)
        {
            auto els = _spatial.get_active();

            for (auto it = els.rbegin(); it != els.rend(); ++it)
            {
                const auto& spr = _spatial[*it];

                if (spr._bbox.contains(position))
                {
                    if (spr._sprite.is_alpha_visible(position.x - spr._bbox.x, position.y - spr._bbox.y))
                        return *it;
                }
            }
            return -1;
        }

        bool EditActive()
        {
            return false;
        }

        bool ImguiWorkspaceMenu(ImGui::CanvasParams& canvas) override
        {
            BeginDefaultMenu("wsmnu", canvas);
            if (EndDefaultMenu(canvas))
            {
            }
            return false;
        }

        bool ImguiWorkspace(ImGui::CanvasParams& canvas) override
        {
            _edit._sprite = {};

            if (IsHidden())
                return false;

            bool modified = false;

            ImVec2 mouse_pos = canvas.ScreenToWorld(ImGui::GetIO().MousePos);
            if (ImGui::IsItemClicked(0))
            {
                _select = FindActiveAt(mouse_pos);
                if (_select != -1)
                {
                    ImVec2 pos{_spatial[_select]._bbox.x, _spatial[_select]._bbox.y};
                    canvas.BeginDrag(pos, (void*)(size_t)_select);
                    modified = true;
                }
            }
            if (ImGui::IsItemClicked(1))
            {
                _select = -1;
            }

            if (_select >= 0)
            {
                ImVec2 pos{_spatial[_select]._bbox.x, _spatial[_select]._bbox.y};
                if (canvas.EndDrag(pos, (void*)(size_t)_select))
                {
                    MoveTo(_select, pos);
                    modified = true;
                }
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE",
                                                                               ImGuiDragDropFlags_AcceptPeekOnly |
                                                                                   ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<Atlas::Pack*>(ImGui::GetDragData("SPRITE")))
                    {
                        _edit._sprite      = *object;
                        _edit._bbox.width  = _edit._sprite.sprite->_source.width;
                        _edit._bbox.height = _edit._sprite.sprite->_source.height;
                        _edit._bbox.x      = mouse_pos.x;
                        _edit._bbox.y      = mouse_pos.y;
                        _edit._index       = _max_index;
                    }
                }
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE",
                                                                               ImGuiDragDropFlags_AcceptNoPreviewTooltip))
                {
                    if (auto object = static_cast<Atlas::Pack*>(ImGui::GetDragData("SPRITE")))
                    {
                        _edit._sprite      = *object;
                        _edit._bbox.width  = _edit._sprite.sprite->_source.width;
                        _edit._bbox.height = _edit._sprite.sprite->_source.height;
                        _edit._bbox.x      = mouse_pos.x;
                        _edit._bbox.y      = mouse_pos.y;
                        _edit._index       = _max_index;
                        ++_max_index;
                        _spatial.insert(_edit);
                        modified = true;
                    }
                }

                ImGui::EndDragDropTarget();
            }


            if (uint32_t(_select) < (uint32_t)_spatial.size())
            {
                auto* dc  = ImGui::GetWindowDrawList();
                auto& nde = _spatial[_select];
                ImVec2 min{nde._bbox.x, nde._bbox.y};
                ImVec2 max{nde._bbox.x2(), nde._bbox.y2()};
                dc->AddRect(canvas.WorldToScreen(min),
                            canvas.WorldToScreen(max),
                            IM_COL32(255, 255, 255, 255),
                            0,
                            ImDrawFlags_Closed,
                            1.0f);
            }

            if (gSettings.visible_grid)
            {
                auto* dc = ImGui::GetWindowDrawList();
                auto cb = [&dc, &canvas](const Rectf& rc)
                {
                    ImVec2 min{rc.x, rc.y};
                    ImVec2 max{rc.x2(), rc.y2()};
                    dc->AddRect(canvas.WorldToScreen(min),
                                canvas.WorldToScreen(max),
                                IM_COL32(255, 255, 0, 190),
                                0,
                                ImDrawFlags_Closed,
                                1.0f);
                };
                _spatial.for_each_node(cb);
            }

            return modified;
        }

        void ImguiSetup() override
        {
            SceneLayer::ImguiSetup();
            ImGui::Checkbox("Sort by Y", &_sort_y);
        }

        bool ImguiUpdate(bool items) override
        {
            if (!items)
                return EditActive();

            bool modified = false;
            ImGui::LineItem(ImGui::GetID(this), {-1, ImGui::GetFrameHeightWithSpacing()})
                .Space()
                .PushStyle(ImStyle_Button, 10, gSettings.list_visible_items)
                .Text(gSettings.list_visible_items ? " " ICON_FA_EYE " " : " " ICON_FA_EYE_SLASH " ")
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 1, false)
                .Text(" " ICON_FA_BAN " ")
                .PopStyle()
                .End();

            if (ImGui::Line().Return())
            {
                if (_select != -1 && ImGui::Line().HoverId() == 1)
                {
                    Destroy(_select);
                    _select = -1;
                    modified = true;
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    gSettings.list_visible_items = !gSettings.list_visible_items;
                }
            }

            if (IsKeyPressed(KEY_DELETE))
            {
                if (_select != -1)
                {
                    Destroy(_select);
                    _select = -1;
                }
            }

            if (ImGui::BeginChildFrame(ImGui::GetID("sprpt"), {-1, -1}, 0))
            {
                if (gSettings.list_visible_items)
                {
                    for (auto n : _spatial.get_active())
                    {
                        ImGui::PushID(n);
                        auto& el = _spatial[n];
                        if (el._sprite.sprite)
                        {
                            if (ImGui::Selectable(el._sprite.sprite->_name.c_str(), n == _select))
                            {
                                _select = n;
                            }
                        }
                        ImGui::PopID();
                    }
                }
                else
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(_spatial.size());
                    while (clipper.Step())
                    {
                        for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                        {
                            ImGui::PushID(n);
                            if (!_spatial.is_empty(n))
                            {
                                auto& el = _spatial[n];
                                if (el._sprite.sprite)
                                {
                                    if (ImGui::Selectable(el._sprite.sprite->_name.c_str(), n == _select))
                                    {
                                        _select = n;
                                    }
                                }
                            }
                            else
                            {
                                ImGui::Selectable(" ");
                            }

                            ImGui::PopID();
                        }
                    }
                }
            }
            ImGui::EndChildFrame();

            return modified;
        }

    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        Node                                                                                 _edit;
        int32_t                                                                              _select    = -1;
        uint32_t                                                                             _max_index = 0;
        bool                                                                                 _sort_y    = false;
    };

    SceneLayer* SceneLayer::CreateSprite()
    {
        return new SpriteSceneLayer;
    }

}
