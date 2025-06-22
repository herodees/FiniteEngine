#include "application.hpp"
#include "editor/imgui_control.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "scene_layer.hpp"
#include "utils/imguiline.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include <rlgl.h>

namespace fin
{
    class RegionSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            msg::Var       _points;
            Rectf          _bbox;
            Texture2D::Ptr _texture;
            Vec2i          _offset;
            int32_t        _index{0};
            bool           _need_update{};

            bool     operator==(const Node& ot) const
            {
                return this == &ot;
            }

            uint32_t Size() const
            {
                return _points.size() / 2;
            }

            bool HitTest(float x, float y) const
            {
                if (Size() < 3)
                    return false; // Not a valid polygon

                bool   inside = false;
                uint32_t n      = Size();

                for (uint32_t i = 0, j = n - 1; i < n; j = i++)
                {
                    auto p1 = GetPoint(i);
                    auto p2 = GetPoint(j);

                    // Check if point is exactly on a vertex
                    if ((p1.x == x && p1.y == y) || (p2.x == x && p2.y == y))
                    {
                        return true;
                    }

                    // Check if point is on a horizontal edge
                    if ((p1.y == p2.y) && (p1.y == y) && (x > std::min(p1.x, p2.x)) && (x < std::max(p1.x, p2.x)))
                    {
                        return true;
                    }

                    // Check if point is on a vertical edge
                    if ((p1.x == p2.x) && (p1.x == x) && (y > std::min(p1.y, p2.y)) && (y < std::max(p1.y, p2.y)))
                    {
                        return true;
                    }

                    // Ray casting intersection check
                    if (((p1.y > y) != (p2.y > y)) && (x < (p2.x - p1.x) * (y - p1.y) / (p2.y - p1.y) + p1.x))
                    {
                        inside = !inside;
                    }
                }
                return inside;
            }

            Vec2f GetPoint(uint32_t n) const
            {
                auto self = const_cast<Node*>(this);
                return {self->_points[n * 2].get(0.f), self->_points[n * 2 + 1].get(0.f)};
            }

            void SetPoint(uint32_t n, Vec2f val)
            {
                _need_update = true;
                _points.set_item(n * 2, val.x);
                _points.set_item(n * 2 + 1, val.y);
            }

            Node& Insert(Vec2f pt, int32_t n)
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

            int32_t Find(Vec2f pt, float radius)
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

            void Update()
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

        RegionSceneLayer() : SceneLayer(LayerType::Region), _spatial({})
        {
            GetName() = "RegionLayer";
            GetIcon() = ICON_FA_MAP_LOCATION_DOT;
            _color = 0xff50ffc0;
        };

        void Resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void Activate(const Rectf& region) override
        {
            SceneLayer::Activate(region);

            _spatial.activate(region);

            _spatial.sort_active([&](int a, int b) { return _spatial[a]._index < _spatial[b]._index; });
        }

        void Serialize(msg::Var& ar)
        {
            SceneLayer::Serialize(ar);
            msg::Var items;
            items.make_array(_spatial.size());

            auto Save = [&items](Node& node)
            {
                msg::Var item;
                item.make_object(1);
                item.set_item("p", node._points.clone());
                item.set_item("i", node._index);
                if (node._texture)
                {
                    item.set_item("t", node._texture->get_path());
                }
                if (node._offset.x)
                {
                    item.set_item("tx", node._offset.x);
                }
                if (node._offset.y)
                {
                    item.set_item("ty", node._offset.y);
                }

                items.push_back(item);
            };

            _spatial.for_each(Save);
            ar.set_item("items", items);
        }

        void Deserialize(msg::Var& ar)
        {
            SceneLayer::Deserialize(ar);
            _spatial.clear();

            auto els = ar["items"];
            _max_index = ar["max_index"].get(0);

            for (auto& el : els.elements())
            {
                Node nde;
                nde._points = el["p"].clone();
                auto idx    = el["i"];
                if (idx.is_undefined())
                {
                    nde._index = ++_max_index;
                }
                else
                {
                    nde._index = idx.get(0);
                }


                auto tx = el["t"];
                if (tx.is_string())
                {
                    nde._texture = Texture2D::load_shared(tx.str());
                    nde._texture->set_repeat(true);

                    nde._offset.x = el["tx"].get(0);
                    nde._offset.y = el["ty"].get(0);
                }

                nde.Update();
                _spatial.insert(nde);
            }
        }

        int FindAt(Vec2f position)
        {
            static std::vector<int32_t> nodes;
            nodes.clear();

            _spatial.query(Rectf{position.x, position.y, 1, 1}, nodes);

            if (nodes.empty())
                return -1;

            int32_t max_index = INT_MIN;
            int32_t selected = -1;
            for (auto idx : nodes)
            {
                if (_spatial[idx].HitTest(position.x, position.y))
                {
                    if (max_index < _spatial[idx]._index)
                    {
                        selected = idx;
                        max_index = _spatial[idx]._index;
                    }
                }
            }

            return selected;
        }

        void MoveTo(int obj, Vec2f pos)
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
            o.Update();

            _spatial.insert(o);
        }

        void RenderGrid(Renderer& dc)
        {
            const int startX = std::max(0.f, _region.x / TileSize);
            const int startY = std::max(0.f, _region.y / TileSize);

            const int endX = (_region.x2() / TileSize) + 2;
            const int endY = (_region.y2() / TileSize) + 2;

            auto minpos = Vec2i{startX, startY};
            auto maxpos = Vec2i{endX, endY};

            Color clr{255, 255, 0, 190};
            dc.set_color(clr);

            for (int y = minpos.y; y < maxpos.y; ++y)
            {
                dc.render_line((float)minpos.x * TileSize, (float)y * TileSize, (float)maxpos.x * TileSize, (float)y * TileSize);
            }

            for (int x = minpos.x; x < maxpos.x; ++x)
            {
                dc.render_line((float)x * TileSize, (float)minpos.y * TileSize, (float)x * TileSize, (float)maxpos.y * TileSize);
            }
        }

        void Update(float dt) override
        {
        }

        void Render(Renderer& dc) override
        {
            if (IsHidden())
                return;

            rlDisableBackfaceCulling();
            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];
                if (nde._texture)
                {
                    const auto center = nde._bbox.center();
                    const auto width  = nde._texture->get_texture()->width;
                    const auto height = nde._texture->get_texture()->height;

                    rlBegin(RL_QUADS);
                    rlSetTexture(nde._texture->get_texture()->id);
                    rlColor4ub(255, 255, 255, 255);
                    rlNormal3f(0.0f, 0.0f, 1.0f);

                    for (uint32_t n = 0; n < nde.Size(); ++n)
                    {
                        auto pt1 = nde.GetPoint(n);
                        auto pt2 = nde.GetPoint((n + 1) % nde.Size());

                        rlTexCoord2f((pt1.x - nde._offset.x) / width, (pt1.y - nde._offset.y) / height);
                        rlVertex2f(pt1.x, pt1.y);

                        rlTexCoord2f((pt2.x - nde._offset.x) / width, (pt2.y - nde._offset.y) / height);
                        rlVertex2f(pt2.x, pt2.y);

                        rlTexCoord2f((center.x - nde._offset.x) / width, (center.y - nde._offset.y) / height);
                        rlVertex2f(center.x, center.y);

                        rlTexCoord2f((center.x - nde._offset.x) / width, (center.y - nde._offset.y) / height);
                        rlVertex2f(center.x, center.y);
                    }

                    rlEnd();

                }
            }
            rlSetTexture(0);
        }

        bool ImguiWorkspaceMenu(ImGui::CanvasParams& canvas) override
        {
            BeginDefaultMenu("wsmnu", canvas);
            ImGui::Line()

                .PushStyle(ImStyle_Button, 10, _edit_region)
                .Text(ICON_FA_ARROW_POINTER " Edit")
                .PopStyle()
                .Space()
                .PushStyle(ImStyle_Button, 20, !_edit_region)
                .Text(ICON_FA_BRUSH " Create")
                .PopStyle();

            if (EndDefaultMenu(canvas))
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
            return false;
        }

        bool ImguiWorkspace(ImGui::CanvasParams& canvas) override
        {
            if (IsHidden())
                return false;

            bool   modified  = false;
            ImVec2 mouse_pos = canvas.ScreenToWorld(ImGui::GetIO().MousePos);
            // Edit region points
            if (_edit_region)
            {
                if (auto* reg = selected_region())
                {
                    if (ImGui::IsItemClicked(0))
                    {
                        _active_point = reg->Find(mouse_pos, 5 / canvas.zoom);
                        if (_active_point != -1)
                        {
                            ImVec2 pt = {reg->GetPoint(_active_point).x, reg->GetPoint(_active_point).y};
                            canvas.BeginDrag(pt, (void*)(size_t)_active_point);
                            modified = true;
                        }
                    }
                    if (ImGui::IsItemClicked(1))
                    {
                        _selected = -1;
                    }

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        // Find nearest point on edge
                        int    insert_index = -1;
                        ImVec2 insert_point;
                        float  nearest_dist = 16.0f * 16.f;

                        for (int i = 0; i < reg->Size(); ++i)
                        {
                            ImVec2 a = (ImVec2&)reg->GetPoint(i);
                            ImVec2 b = (ImVec2&)reg->GetPoint((i + 1) % reg->Size());

                            // Project mouse_pos onto segment a-b
                            ImVec2 ab   = b - a;
                            ImVec2 ap   = mouse_pos - a;
                            float  t    = ImClamp(ImDot(ap, ab) / ImLengthSqr(ab), 0.0f, 1.0f);
                            ImVec2 proj = a + ab * t;
                            float  dist = ImLengthSqr(mouse_pos - proj); 

                            if (dist < nearest_dist)
                            {
                                nearest_dist = dist;
                                insert_point = proj;
                                insert_index = i + 1;
                            }
                        }

                        if (insert_index != -1)
                        {
                            // Insert the point
                            reg->Insert(insert_point, insert_index);
                            _active_point = insert_index;
                            canvas.BeginDrag(insert_point, (void*)(size_t)_active_point);
                            modified = true;
                        }
                    }
                }
                else if (ImGui::IsItemClicked(0))
                {
                    _selected = FindAt(mouse_pos);
                }

                if (_active_point != -1)
                {
                    if (auto* reg = selected_region())
                    {
                        ImVec2 pt = {reg->GetPoint(_active_point).x, reg->GetPoint(_active_point).y};
                        if (canvas.EndDrag(pt, (void*)(size_t)_active_point))
                        {
                            auto obj = *reg;
                            _spatial.remove(*selected_region());
                            _selected = -1;
                            obj.SetPoint(_active_point, pt);
                            obj.Update();
                            _selected = _spatial.insert(obj);
                            modified  = true;
                        }
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
                        obj.Insert(mouse_pos, _active_point + 1);
                        obj.Update();
                        _spatial.insert(obj);
                        _active_point = _active_point + 1;
                        modified      = true;
                    }
                    else
                    {
                        Node obj;
                        obj.Insert(mouse_pos, 0);
                        obj.Update();
                        obj._index    = ++_max_index;
                        _selected     = _spatial.insert(obj);
                        _active_point = 0;
                        modified      = true;
                    }
                }
                if (ImGui::IsItemClicked(1))
                {
                    _active_point = -1;
                    _selected     = -1;
                }
            }

            auto* selected = selected_region();
            auto* dc = ImGui::GetWindowDrawList();
            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];
                for (uint32_t i = 0; i < nde.Size(); ++i)
                {
                    auto pt1 = canvas.WorldToScreen((ImVec2&)nde.GetPoint(i));
                    auto pt2 = canvas.WorldToScreen((ImVec2&)nde.GetPoint((i + 1) % nde.Size()));
                    if (selected && selected == &nde)
                    {
                        if (i == _active_point)
                        {
                            dc->AddCircleFilled(pt1, 5, 0xff00ff00);
                        }
                        else
                        {
                            dc->AddCircleFilled(pt1, 3, 0xff00ffff);
                        }
                    }
                    dc->AddLine(pt1, pt2, 0xff0000ff);
                }
            }

            if (gSettings.visible_grid)
            {
                auto  cb = [&dc, &canvas](const Rectf& rc)
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

        bool EditActive()
        {
            bool modified = false;
            if (auto* reg = selected_region())
            {
        
                modified |= ImGui::PointVector("Points", &reg->_points, {-1, ImGui::GetFrameHeightWithSpacing() * 4}, &_active_point);
                modified |= ImGui::TextureInput("Texture", &reg->_texture);
                if (reg->_texture)
                    modified |= ImGui::DragInt2("Offset", &reg->_offset.x);

                modified |= ImGui::DragInt("Index", &reg->_index, 1, 0, _max_index + 1, "%d");

                if (modified)
                {
                    reg->Update();
                }
            }
            return modified;
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
                if (ImGui::Line().HoverId() == 1)
                {
                    if (auto* reg = selected_region())
                    {
                        _spatial.remove(*reg);
                        modified = true;
                    }
                }
                if (ImGui::Line().HoverId() == 10)
                {
                    gSettings.list_visible_items = !gSettings.list_visible_items;
                }
            }

            if (ImGui::BeginChildFrame(ImGui::GetID("regpt"), {-1, -1}, 0))
            {
                if (gSettings.list_visible_items)
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
                    clipper.Begin(_spatial.size(), ImGui::GetFrameHeight());
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

            return modified;
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
        int32_t                                                                              _max_index = 0;
        int32_t                                                                              _edit         = -1;
        int32_t                                                                              _selected     = -1;
        int32_t                                                                              _active_point = -1;
        bool                                                                                 _edit_region{true};
    };

    SceneLayer* SceneLayer::CreateRegion()
    {
        return new RegionSceneLayer;
    }
}
