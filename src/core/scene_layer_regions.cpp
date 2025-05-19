#include "application.hpp"
#include "editor/imgui_control.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "scene_layer.hpp"
#include "scene_object.hpp"
#include "utils/imguiline.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"

namespace fin
{
    class RegionSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            msg::Var _points;
            Rectf    _bbox;
            bool     _need_update{};
            bool     operator==(const Node& ot) const
            {
                return this == &ot;
            }

            uint32_t size() const
            {
                return _points.size() / 2;
            }

            Vec2f get_point(uint32_t n)
            {
                return {_points[n * 2].get(0.f), _points[n * 2 + 1].get(0.f)};
            }

            void set_point(uint32_t n, Vec2f val)
            {
                _need_update = true;
                _points.set_item(n * 2, val.x);
                _points.set_item(n * 2 + 1, val.y);
            }

            Node& insert(Vec2f pt, int32_t n)
            {
                _need_update = true;
                if (uint32_t(n * 2) >= _points.size())
                {
                    _points.push_back(pt.x);
                    _points.push_back(pt.y);
                }
                else
                {
                    n = n * 2;
                    _points.insert(n, pt.x);
                    _points.insert(n + 1, pt.y);
                }
                return *this;
            }

            int32_t find(Vec2f pt, float radius)
            {
                auto sze = _points.size();
                for (uint32_t n = 0; n < sze; n += 2)
                {
                    Vec2f pos(_points.get_item(n).get(0.f), _points.get_item(n + 1).get(0.f));
                    if (pos.distance_squared(pt) <= (radius * radius))
                    {
                        return n / 2;
                    }
                }
                return -1;
            }

            void update()
            {
                _need_update = false;
                _bbox        = {};
                if (_points.size() >= 2)
                {
                    Region<float> reg(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
                    for (uint32_t n = 0; n < _points.size(); n += 2)
                    {
                        const float nxo = _points.get_item(n).get(0.f);
                        const float nyo = _points.get_item(n + 1).get(0.f);

                        if (nxo < reg.x1)
                            reg.x1 = nxo;

                        if (nxo > reg.x2)
                            reg.x2 = nxo;

                        if (nyo < reg.y1)
                            reg.y1 = nyo;

                        if (nyo > reg.y2)
                            reg.y2 = nyo;
                    }

                    _bbox = reg.rect();
                }
            }
        };

        RegionSceneLayer() : SceneLayer(SceneLayer::Type::Region), _spatial({})
        {
            name() = "RegionLayer";
            icon() = ICON_FA_MAP_LOCATION_DOT;
            _color = 0xff50ffc0;
        };

        void resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void activate(const Rectf& region) override
        {
            SceneLayer::activate(region);
            _spatial.activate(region);
        }

        void serialize(msg::Writer& ar)
        {
            SceneLayer::serialize(ar);

            auto save = [&ar](Node& node)
            {
                ar.begin_object();
                ar.key("p");
                ar.begin_array();
                for (auto el : node._points.elements())
                    ar.value(el.get(0.f));
                ar.end_array();
                ar.end_object();
            };

            ar.key("items");
            ar.begin_array();
            _spatial.for_each(save);
            ar.end_array();
        }

        void deserialize(msg::Value& ar)
        {
            SceneLayer::deserialize(ar);
            _spatial.clear();

            auto els = ar["items"];
            for (auto& el : els.elements())
            {
                Node nde;
                auto pts = el["p"];
                for (auto& el : pts.elements())
                {
                    nde._points.push_back(el.get(0.f));
                }
                nde.update();
                _spatial.insert(nde);
            }
        }

        int find_at(Vec2f position)
        {
            return _spatial.find_at(position.x, position.y);
        }

        void moveto(int obj, Vec2f pos)
        {
            auto o = _spatial[obj];
            _spatial.remove(_spatial[obj]);
            if (o._points.size() >= 2)
            {
                const float xo = o._points.get_item(0).get(0.f);
                const float yo = o._points.get_item(1).get(0.f);

                for (uint32_t n = 0; n < o._points.size(); n += 2)
                {
                    const float nxo = o._points.get_item(n).get(0.f) - xo + pos.x;
                    const float nyo = o._points.get_item(n + 1).get(0.f) - yo + pos.y;

                    o._points.set_item(n, nxo);
                    o._points.set_item(n + 1, nyo);
                }
            }
            o.update();

            _spatial.insert(o);
        }

        void render_edit(Renderer& dc) override
        {
            dc.set_color(WHITE);

            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];

                auto sze = nde.size();
                for (auto idx = 0; idx < sze; ++idx)
                {
                    dc.render_line(nde.get_point(idx), nde.get_point((idx + 1) % sze));
                }
            }


            if (auto* reg = selected_region())
            {
                dc.set_color({255, 255, 0, 255});
                dc.render_line_rect(reg->_bbox);

                for (auto n = 0; n < reg->size(); ++n)
                {
                    auto pt = reg->get_point(n);
                    if (_active_point == n)
                        dc.render_circle(pt, 3);
                    else
                        dc.render_line_circle(pt, 3);
                }
            }

            if (g_settings.visible_grid)
            {
                render_grid(dc);
            }
        }

        void render_grid(Renderer& dc)
        {
            const int startX = std::max(0.f, _region.x / tile_size);
            const int startY = std::max(0.f, _region.y / tile_size);

            const int endX = (_region.x2() / tile_size) + 2;
            const int endY = (_region.y2() / tile_size) + 2;

            auto minpos = Vec2i{startX, startY};
            auto maxpos = Vec2i{endX, endY};

            Color clr{255, 255, 0, 190};
            dc.set_color(clr);

            for (int y = minpos.y; y < maxpos.y; ++y)
            {
                dc.render_line((float)minpos.x * tile_size,
                               (float)y * tile_size,
                               (float)maxpos.x * tile_size,
                               (float)y * tile_size);
            }

            for (int x = minpos.x; x < maxpos.x; ++x)
            {
                dc.render_line((float)x * tile_size,
                               (float)minpos.y * tile_size,
                               (float)x * tile_size,
                               (float)maxpos.y * tile_size);
            }
        }

        void update(float dt) override
        {
        }

        void imgui_workspace_menu() override
        {
            BeginDefaultMenu("wsmnu");
            ImGui::Line()
                .PushStyle(ImStyle_Button, 10, _edit_region)
                .Text(ICON_FA_ARROW_POINTER " Edit")
                .PopStyle()
                .Space()
                .PushStyle(ImStyle_Button, 20, !_edit_region)
                .Text(ICON_FA_BRUSH " Create")
                .PopStyle();

            if (EndDefaultMenu())
            {
                if (ImGui::Line().HoverId() == 10)
                {
                    _edit_region = true;
                }
                if (ImGui::Line().HoverId() == 20)
                {
                    _edit_region = false;
                }
            }
        }

        void imgui_workspace(Params& params, DragData& drag) override
        {
            // Edit region points
            if (_edit_region)
            {
                if (auto* reg = selected_region())
                {
                    if (ImGui::IsItemClicked(0))
                    {
                        _active_point = reg->find(params.mouse, 5);
                    }
                    if (ImGui::IsItemClicked(1))
                    {
                        _selected = -1;
                    }
                }
                else if (ImGui::IsItemClicked(0))
                {
                    _selected = find_at(params.mouse);
                }

                if (drag._active && _active_point != -1)
                {
                    if (auto* reg = selected_region())
                    {
                        auto obj = *reg;
                        _spatial.remove(*selected_region());
                        auto pt = obj.get_point(_active_point) + drag._delta;
                        obj.set_point(_active_point, pt);
                        obj.update();
                        _spatial.insert(obj);
                    }
                }
            }
            // Create region
            else
            {
                if (ImGui::IsItemClicked(0))
                {
                    if (auto* reg = selected_region())
                    {
                        auto obj = *reg;
                        _spatial.remove(*reg);
                        obj.insert(params.mouse, _active_point + 1);
                        obj.update();
                        _spatial.insert(obj);
                        _active_point = _active_point + 1;
                    }
                    else
                    {
                        Node obj;
                        obj.insert(params.mouse, 0);
                        obj.update();
                        _selected     = _spatial.insert(obj);
                        _active_point = 0;
                    }
                }
                if (ImGui::IsItemClicked(1))
                {
                    _active_point = -1;
                    _selected     = -1;
                }
            }
        }

        void edit_active()
        {
            if (auto* reg = selected_region())
            {
            }
        }

        void imgui_update(bool items) override
        {
            if (!items)
                return edit_active();

            ImGui::LineItem(ImGui::GetID(this), {-1, ImGui::GetFrameHeightWithSpacing()})
                .Space()
                .PushStyle(ImStyle_Button, 10, g_settings.list_visible_items)
                .Text(g_settings.list_visible_items ? " " ICON_FA_EYE " " : " " ICON_FA_EYE_SLASH " ")
                .PopStyle()
                .Spring()
                .PushStyle(ImStyle_Button, 1, false)
                .Text(" " ICON_FA_BAN " ")
                .PopStyle()
                .End();

            if (ImGui::Line().Return())
            {
                if (ImGui::Line().HoverId() == 1)
                {
                    if (auto* reg = selected_region())
                    {
                        _spatial.remove(*reg);
                    }
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    g_settings.list_visible_items = !g_settings.list_visible_items;
                }
            }

            if (ImGui::BeginChildFrame(-1, {-1, 250}, 0))
            {
                if (g_settings.list_visible_items)
                {
                    for (auto n : _spatial.get_active())
                    {
                        ImGui::PushID(n);
                        auto& el = _spatial[n];
                        if (el._points.size())
                        {
                            if (ImGui::Selectable(ImGui::FormatStr("Region [%d]", el._points.size() / 2), n == _selected))
                            {
                                _selected = n;
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
                            auto& el = _spatial[n];
                            if (el._points.size())
                            {
                                if (ImGui::Selectable(ImGui::FormatStr("Region [%d]", el._points.size() / 2), n == _selected))
                                {
                                    _selected = n;
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                }
            }

            ImGui::EndChildFrame();
        }

    private:
        Node* selected_region()
        {
            if (size_t(_selected) < size_t(_spatial.size()))
            {
                return &_spatial[_selected];
            }
            return nullptr;
        }

        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        int32_t                                                                              _edit         = -1;
        int32_t                                                                              _selected     = -1;
        int32_t                                                                              _active_point = -1;
        bool                                                                                 _edit_region{};
    };

    SceneLayer* SceneLayer::create_region()
    {
        return new RegionSceneLayer;
    }
}
