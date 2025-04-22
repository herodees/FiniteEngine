#include "scene_object.hpp"

#include "scene.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{

    SceneFactory* s_factory{};

    void SceneObject::serialize(msg::Writer& ar)
    {
        ar.member(Sc::Uid, _prefab_uid);
        ar.member("x", _position.x);
        ar.member("y", _position.y);
        ar.member("fl", _flag);
    }

    void SceneObject::deserialize(msg::Value& ar)
    {
        _position.x = ar["x"].get(_position.x);
        _position.y = ar["y"].get(_position.y);
        _flag       = ar["fl"].get(_flag);
    }

    void SceneObject::move_to(Vec2f pos)
    {
        _position = pos;
        _scene->_spatial_db.update_for_new_location(this);
    }

    Atlas::Pack& SceneObject::set_sprite(std::string_view path, std::string_view spr)
    {
        _img = Atlas::load_shared(path, spr);
        return _img;
    }

    Atlas::Pack& SceneObject::sprite()
    {
        return _img;
    }

    msg::Var& SceneObject::collision()
    {
        return _collision;
    }

    uint64_t SceneObject::prefab() const
    {
        return _prefab_uid;
    }

    Line<float> SceneObject::iso() const
    {
        return Line<float>(_iso.point1 + _position, _iso.point2 + _position);
    }

    Region<float> SceneObject::bounding_box() const
    {
        Region<float> out(_position.x, _position.y, _position.x, _position.y);
        if (_img.sprite)
        {
            out.x1 -= _img.sprite->_origina.x;
            out.y1 -= _img.sprite->_origina.y;
            out.x2 = out.x1 + _img.sprite->_source.width;
            out.y2 = out.y1 + _img.sprite->_source.height;
        }
        return out;
    }

    void SceneObject::render(Renderer& dc)
    {
        if (!_img.sprite)
            return;

        Rectf dest;
        dest.x      = _position.x - _img.sprite->_origina.x;
        dest.y      = _position.y - _img.sprite->_origina.y;
        dest.width  = _img.sprite->_source.width;
        dest.height = _img.sprite->_source.height;

        dc.render_texture(_img.sprite->_texture, _img.sprite->_source, dest);
    }

    void SceneObject::edit_render(Renderer& dc)
    {
        if (!_img.sprite)
            return;

        Region<float> bbox(bounding_box());

        dc.set_color(RED);
        dc.render_line({bbox.x1, bbox.y1}, {bbox.x1, bbox.y2});
        dc.render_line({bbox.x1, bbox.y2}, {bbox.x2, bbox.y2});
        dc.render_line({bbox.x2, bbox.y2}, {bbox.x2, bbox.y1});
        dc.render_line({bbox.x2, bbox.y1}, {bbox.x1, bbox.y1});
    }

    bool SceneObject::edit_update()
    {
        if (!ImGui::CollapsingHeader("Scene object", ImGuiTreeNodeFlags_DefaultOpen))
            return false;

        bool modified = false;

        modified |= ImGui::CheckboxFlags("Disabled", &_flag, SceneObjectFlag::Disabled);
        modified |= ImGui::CheckboxFlags("Hidden", &_flag, SceneObjectFlag::Hidden);

        modified |= ImGui::SpriteInput("Sprite", &_img);
        modified |= ImGui::PointVector("Collision", &_collision, {-1, ImGui::GetFrameHeightWithSpacing() * 4});

        return modified;
    }

    void SceneObject::save(msg::Var& ar)
    {
        ar.set_item(Sc::Uid, _prefab_uid);
        ar.set_item(Sc::Class, object_type());
        ar.set_item("fl", _flag);
        auto iso = msg::Var::array(4);
        iso.push_back(_iso.point1.x);
        iso.push_back(_iso.point1.y);
        iso.push_back(_iso.point2.x);
        iso.push_back(_iso.point2.y);
        ar.set_item("iso", iso);
        if (_collision.size())
        {
            ar.set_item("coll", _collision.clone());
        }
        if (_img.atlas)
        {
            ar.set_item("atl", _img.atlas->get_path());
            if (_img.sprite)
            {
                ar.set_item("spr", _img.sprite->_name);
            }
        }
    }

    void SceneObject::load(msg::Var& ar)
    {
        _prefab_uid     = ar[Sc::Uid].get(0ull);
        _flag       = ar["fl"].get(0);
        _collision  = ar["coll"].clone();
        set_sprite(ar["atl"].str(), ar["spr"].str());
        auto iso      = ar.get_item("iso");
        _iso.point1.x = iso.get_item(0).get(0.f);
        _iso.point1.y = iso.get_item(1).get(0.f);
        _iso.point2.x = iso.get_item(2).get(0.f);
        _iso.point2.y = iso.get_item(3).get(0.f);
    }



    void SceneObject::move(Vec2f pos)
    {
        move_to(_position + pos);
    }

    bool SceneObject::flag_get(SceneObjectFlag f) const
    {
        return _flag & f;
    }

    void SceneObject::flag_reset(SceneObjectFlag f)
    {
        _flag &= ~f;
    }

    void SceneObject::flag_set(SceneObjectFlag f)
    {
        _flag |= f;
    }
    bool SceneObject::is_disabled() const
    {
        return flag_get(SceneObjectFlag::Disabled);
    }

    void SceneObject::disable(bool v)
    {
        v ? flag_set(SceneObjectFlag::Disabled) : flag_reset(SceneObjectFlag::Disabled);
    }

    bool SceneObject::is_hidden() const
    {
        return flag_get(SceneObjectFlag::Hidden);
    }

    void SceneObject::hide(bool v)
    {
        v ? flag_set(SceneObjectFlag::Hidden) : flag_reset(SceneObjectFlag::Hidden);
    }

    Scene* SceneObject::scene()
    {
        return _scene;
    }

    Vec2f SceneObject::position() const
    {
        return _position;
    }


    SceneFactory::SceneFactory()
    {
        s_factory = this;
    }

    SceneFactory& SceneFactory::instance()
    {
        return *s_factory;
    }

    SceneObject* SceneFactory::create(std::string_view type) const
    {
        auto it = _factory.find(type);
        if (it == _factory.end())
            return nullptr;
        return it->second.fn();
    }

    SceneObject* SceneFactory::create(uint64_t uid) const
    {
        auto it = _prefab.find(uid);
        if (it == _prefab.end())
            return nullptr;
        auto val = it->second;
        auto cls = val.get_item(Sc::Class);
        if (auto* obj = create(cls.str()))
        {
            obj->load(val);
            return obj;
        }
        return nullptr;
    }

    bool SceneFactory::load_prototypes(std::string_view type)
    {
        auto it = _factory.find(type);
        if (it == _factory.end())
            return false;

        std::string str = _base_folder;
        str.append(type).append(".prefab");

        if (auto* txt = LoadFileText(str.c_str()))
        {
            auto err = it->second.items.from_string(txt);
            for (auto el : it->second.items.elements())
            {
                _prefab[el.get_item(Sc::Uid).get(0ull)] = el;
            }
            UnloadFileText(txt);
            return err == msg::VarError::ok;
        }

        return false;
    }

    bool SceneFactory::save_prototypes(std::string_view type)
    {
        auto it = _factory.find(type);
        if (it == _factory.end())
            return false;

        std::string str = _base_folder;
        str.append(type).append(".prefab");

        std::string txt;
        it->second.items.to_string(txt);

        return SaveFileText(str.c_str(), txt.data());
    }

    void SceneFactory::set_root(const std::string& startPath)
    {
        _base_folder = startPath;
    }

    void SceneFactory::center_view()
    {
        _scroll_to_center = true;
    }

    void SceneFactory::show_workspace()
    {
        if (!_edit || !_edit->is_selected())
            return;

        static int                 s_drag_point = 0;
        static ImVec2              s_prev_mpos;
        static std::vector<ImVec2> s_points;
        if (!ImGui::IsMouseDown(0))
        {
            s_drag_point = 0;
        }

        auto          spr = _edit->obj->sprite();
        Scene::Params params;

        ImVec2 visible_size = ImGui::GetContentRegionAvail();
        params.pos          = ImGui::GetWindowPos();
        auto cur            = ImGui::GetCursorPos();
        params.dc           = ImGui::GetWindowDrawList();
        auto mpos           = ImGui::GetMousePos();

        params.scroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
        params.pos.x -= ImGui::GetScrollX();
        params.pos.y -= ImGui::GetScrollY();

        ImGui::InvisibleButton("Canvas", ImVec2(2000, 2000));
        params.pos.x += 1000;
        params.pos.y += 1000;

        params.mouse = {mpos.x - params.pos.x, mpos.y - params.pos.y};

        if (_scroll_to_center)
        {
            _scroll_to_center = false;
            ImGui::SetScrollHereX();
            ImGui::SetScrollHereY();
        }

        if (spr.sprite)
        {
            Region<float> reg(params.pos.x - spr.sprite->_origina.x,
                              params.pos.y - spr.sprite->_origina.y,
                              params.pos.x + spr.sprite->_source.width - spr.sprite->_origina.x,
                              params.pos.y + spr.sprite->_source.height - spr.sprite->_origina.y);

            ImVec2 txs(spr.sprite->_texture->width, spr.sprite->_texture->height);
            params.dc->AddImage((ImTextureID)spr.sprite->_texture,
                                {reg.x1, reg.y1},
                                {reg.x2, reg.y2},
                                {spr.sprite->_source.x / txs.x, spr.sprite->_source.y / txs.y},
                                {spr.sprite->_source.x2() / txs.x, spr.sprite->_source.y2() / txs.y});

            params.dc->AddRect({reg.x1, reg.y1}, {reg.x2, reg.y2}, 0x7fffff00);

            if (_edit_origin)
            {
                ImVec2 a{params.pos.x + _edit->obj->_iso.point1.x, params.pos.y + _edit->obj->_iso.point1.y};
                ImVec2 b{params.pos.x + _edit->obj->_iso.point2.x, params.pos.y + _edit->obj->_iso.point2.y};

                bool hovera = params.mouse_distance2(a - params.pos) < 5 * 5;
                bool hoverb = params.mouse_distance2(b - params.pos) < 5 * 5;

                if (ImGui::IsItemClicked(0))
                {
                    if (hovera || hoverb)
                    {
                        s_prev_mpos  = params.mouse;
                        s_drag_point = hovera ? 1 : 2;
                    }
                }

                if (s_drag_point)
                {
                    auto diff   = s_prev_mpos - params.mouse;
                    s_prev_mpos = params.mouse;
                    auto& iso   = s_drag_point == 1 ? _edit->obj->_iso.point1 : _edit->obj->_iso.point2;
                    iso -= diff;
                }

                params.dc->AddLine(a, b, 0xff909090, 2);
                hovera ? params.dc->AddCircleFilled(a, 5, 0xff00ff00) : params.dc->AddCircle(a, 5, 0xff00ff00);
                hoverb ? params.dc->AddCircleFilled(b, 5, 0xff0000ff) : params.dc->AddCircle(b, 5, 0xff0000ff);
            }

            if (_edit_collision && _edit->obj->collision().is_array())
            {
                auto coll = _edit->obj->collision();
                s_points.clear();
                for (auto n = 0; n < coll.size(); n += 2)
                {
                    ImVec2 a{params.pos.x + coll.get_item(n).get(0.0f), params.pos.y + coll.get_item(n + 1).get(0.0f)};
                    bool   hovera = params.mouse_distance2(a - params.pos) < 5 * 5;
                    if (ImGui::IsItemClicked(0))
                    {
                        if (hovera)
                        {
                            s_prev_mpos  = params.mouse;
                            s_drag_point = n + 1;
                        }
                    }

                    if (s_drag_point)
                    {
                        auto diff   = s_prev_mpos - params.mouse;
                        s_prev_mpos = params.mouse;
                        coll.set_item(s_drag_point - 1, coll.get_item(s_drag_point - 1).get(0.0f) - diff.x);
                        coll.set_item(s_drag_point, coll.get_item(s_drag_point).get(0.0f) - diff.y);
                    }

                    s_points.push_back(a);
                    hovera ? params.dc->AddCircleFilled(a, 5, 0xff00ff00) : params.dc->AddCircle(a, 5, 0xff00ff00);
                }

                if (s_points.size() >= 3)
                {
                    params.dc->AddPolyline(s_points.data(), (int)s_points.size(), 0xff909090, ImDrawFlags_Closed, 2);
                }
            }
        }

        params.dc->AddLine(params.pos - ImVec2(0, 1000), params.pos + ImVec2(0, 1000), 0x7f00ff00);
        params.dc->AddLine(params.pos - ImVec2(1000, 0), params.pos + ImVec2(1000, 0), 0x7f0000ff);


    }

    void SceneFactory::show_menu()
    {
        if (ImGui::RadioButton("Select", (!_edit_collision && !_edit_origin)))
        {
            _edit_origin    = false;
            _edit_collision = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Origin", _edit_origin))
        {
            _edit_origin    = true;
            _edit_collision = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Collision", _edit_collision))
        {
            _edit_origin    = false;
            _edit_collision = true;
        }
    }

    void SceneFactory::show_properties()
    {
        if (ImGui::BeginTabBar("ScnFactTab"))
        {
            _edit = nullptr;
            for (auto& el : _factory)
            {
                if (ImGui::BeginTabItem(el.second.name.c_str()))
                {
                    _edit = &el.second;

                    if (ImGui::Button(" " ICON_FA_LAPTOP " Add "))
                    {
                        auto obj = msg::Var::object(2);
                        obj.set_item(Sc::Uid, std::generate_unique_id());
                        obj.set_item(Sc::Class, el.first);
                        obj.set_item(Sc::Id, ImGui::FormatStr("%s_%d", el.first.c_str(), el.second.items.size()));
                        el.second.items.push_back(obj);
                        el.second.select(el.second.items.size() - 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(" " ICON_FA_UPLOAD " Save "))
                    {
                        el.second.select(el.second.selected);
                        save_prototypes(el.first);
                    }
                    ImGui::SameLine();
                    ImGui::InvisibleButton("##ib", {-34, 1});
                    ImGui::SameLine();
                    if (ImGui::Button(" " ICON_FA_BAN " "))
                    {
                        if ((uint32_t)el.second.selected < el.second.items.size())
                        {
                            auto sel = el.second.selected;
                            el.second.items.erase(sel);
                            el.second.select(sel);
                        }
                    }

                    if (ImGui::BeginChildFrame(-1, {-1, 100}))
                    {
                        ImGuiListClipper clipper;
                        clipper.Begin(el.second.items.size());
                        while (clipper.Step())
                        {
                            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                            {
                                ImGui::PushID(n);
                                auto val = el.second.items.get_item(n);
                                auto id = val.get_item(Sc::Id);
                                if (ImGui::Selectable(id.c_str(), el.second.selected == n))
                                {
                                    el.second.select(n);
                                }
                                ImGui::PopID();
                            }
                        }
                    }
                    ImGui::EndChildFrame();
                    ImGui::Text("Properties");
                    if (ImGui::BeginChildFrame(-2, {-1, -1}))
                    {
                        show_properties(el.second);
                    }
                    ImGui::EndChildFrame();

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    void SceneFactory::show_explorer()
    {
        if (!ImGui::Begin("Explorer"))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("ExpFactTab"))
        {
            for (auto& [key, info] : _factory)
            {
                if (ImGui::BeginTabItem(info.name.c_str()))
                {
                    if (_explore != &info)
                    {
                        _explore = &info;
                        reset_atlas_cache();
                    }
                   
                    if (ImGui::BeginChildFrame(-1, { -1, -1 }))
                    {
                        _explore = &info;
                        ImGuiListClipper clipper;
                        clipper.Begin(info.items.size());
                        while (clipper.Step())
                        {
                            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                            {
                                ImGui::PushID(n);
                                auto val  = info.items.get_item(n);
                                auto id   = val.get_item(Sc::Id);
                                auto pack = load_atlas(val);

                                if (ImGui::Selectable("##id", info.active == n, 0, {0, 25}))
                                {
                                    info.active = n;
                                }

                                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                                {
                                    info.select(n);
                                    ImGui::SetDragData(info.obj.get(),"PREFAB");
                                    ImGui::SetDragDropPayload("PREFAB", &n, sizeof(int32_t));
                                    ImGui::EndDragDropSource();
                                }

                                if (pack.sprite)
                                {
                                    ImGui::SameLine();
                                    ImGui::SpriteImage(pack.sprite, {25, 25});
                                }
                                ImGui::SameLine();
                                ImGui::Text(id.c_str());

                                ImGui::PopID();
                            }
                        }

                    }
                    ImGui::EndChildFrame();

                    ImGui::EndTabItem();
                }
            }

        }
        ImGui::EndTabBar();

        ImGui::End();
    }

    void SceneFactory::show_properties(ClassInfo& info)
    {
        if (!info.is_selected())
            return;

        auto proto = info.items.get_item(info.selected);
        _buff = proto.get_item(Sc::Id).str();
        if (ImGui::InputText("Id", &_buff))
        {
            proto.set_item(Sc::Id, _buff);
        }

        info.obj->edit_update();
    }

    void SceneFactory::reset_atlas_cache()
    {
        _cache.clear();
    }

    Atlas::Pack SceneFactory::load_atlas(msg::Var& el)
    {
        Atlas::Pack out;
        auto atl = el.get_item("atl");
        auto spr = el.get_item("spr");
        auto it = _cache.find(atl.str());
        if (it == _cache.end())
        {
            out = Atlas::load_shared(atl.str(), spr.str());
            _cache[atl.c_str()] = out.atlas;
        }
        else
        {
            out.atlas = it->second;
            if (auto n = out.atlas->find_sprite(spr.str()))
                out.sprite = &out.atlas->get(n);
        }
        return out;
    }

    void SceneFactory::ClassInfo::select(int32_t n)
    {
        if (is_selected())
        {
            auto ar = items.get_item(selected);
            obj->save(ar);
        }

        selected = n;

        if (is_selected())
        {
            auto ar = items.get_item(selected);
            obj->load(ar);
        }
    }

    bool SceneFactory::ClassInfo::is_selected() const
    {
        return (uint32_t)selected < items.size();
    }

    SceneRegion::SceneRegion()
    {
    }

    SceneRegion::~SceneRegion()
    {
    }

    void SceneRegion::move(Vec2f pos)
    {
        for (uint32_t n = 0; n < _region.size(); n+=2)
        {
            _region.set_item(n, _region.get_item(n).get(0.f) + pos.x);
            _region.set_item(n + 1, _region.get_item(n + 1).get(0.f) + pos.y);
        }
        change();
    }

    void SceneRegion::move_to(Vec2f pos)
    {
        if (_region.size() <= 2)
            return;
        pointf_t origin;
        origin.x = _region.get_item(0).get(0.f);
        origin.y = _region.get_item(1).get(0.f);
        for (uint32_t n = 0; n < _region.size(); n += 2)
        {
            _region.set_item(n, _region.get_item(n).get(0.f) - origin.x + pos.x);
            _region.set_item(n + 1, _region.get_item(n + 1).get(0.f) - origin.y + pos.y);
        }
        change();
    }

    const Region<float>& SceneRegion::bounding_box()
    {
        if (_need_update)
        {
            _bounding_box = {};
            if (_region.size())
            {
                _bounding_box = {FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
                for (uint32_t n = 0; n < _region.size(); n += 2)
                {
                    const auto x = _region.get_item(n).get(0.f);
                    const auto y = _region.get_item(n + 1).get(0.f);

                    if (_bounding_box.x1 > x)
                        _bounding_box.x1 = x;
                    if (_bounding_box.x2 < x)
                        _bounding_box.x2 = x;

                    if (_bounding_box.y1 > y)
                        _bounding_box.y1 = y;
                    if (_bounding_box.y2 < y)
                        _bounding_box.y2 = y;
                }
            }
            _need_update = false;
        }
        return _bounding_box;
    }

    bool SceneRegion::contains(Vec2f point)
    {
        if (!bounding_box().contains(point))
            return false;

        bool    inside = false;
        int32_t n      = get_size();
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            const auto v1 = get_point(i);
            const auto v2 = get_point(j);
            if (((v1.y > point.y) != (v2.y > point.y)) &&
                (point.x < (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x))
            {
                inside = !inside;
            }
        }
        return inside;
    }

    msg::Var& SceneRegion::points()
    {
        return _region;
    }

    void SceneRegion::change()
    {
        _need_update = true;
    }

    int32_t SceneRegion::find_point(Vec2f pt, float radius)
    {
        auto sze = _region.size();
        for (uint32_t n = 0; n < sze; n += 2)
        {
            Vec2f pos(_region.get_item(n).get(0.f), _region.get_item(n + 1).get(0.f));
            if (pos.distance_squared(pt) <= (radius * radius))
            {
                return n / 2;
            }
        }
        return -1;
    }

    SceneRegion& SceneRegion::insert_point(Vec2f pt, int32_t n)
    {
        _need_update = true;
        if (uint32_t(n * 2) >= _region.size())
        {
            _region.push_back(pt.x);
            _region.push_back(pt.y);
        }
        else
        {
            n = n * 2;
            _region.insert(n, pt.x);
            _region.insert(n + 1, pt.y);
        }
        return *this;
    }

    Vec2f SceneRegion::get_point(int32_t n)
    {
        n = n * 2;
        return (uint32_t(n) < _region.size()) ? Vec2f{_region.get_item(n).get(0.f), _region.get_item(n + 1).get(0.f)}
                                              : Vec2f{0, 0};
    }

    void SceneRegion::set_point(Vec2f pt, int32_t n)
    {
        n = n * 2;
        if (uint32_t(n) < _region.size())
        {
            _need_update = true;
            _region.set_item(n, pt.x);
            _region.set_item(n + 1, pt.y);
        }
    }

    int32_t SceneRegion::get_size() const
    {
        return _region.size() / 2;
    }

    void SceneRegion::serialize(msg::Writer& ar)
    {
        ar.key("reg");
        ar.begin_array();
        for (auto el : _region.elements())
        {
            ar.value(el.get(0.f));
        }
        ar.end_array();

    }

    void SceneRegion::deserialize(msg::Value& ar)
    {
        _region.clear();
        for (auto el : ar["reg"].elements())
        {
            _region.push_back(el.get(0.f));
        }
        change();
    }

    bool SceneRegion::edit_update()
    {
        auto modified = false; 
        if (ImGui::PointVector("Points", &_region, {-1, 150}))
        {
            change();
            auto modified = true;
        }
        return modified;
    }

    void SceneRegion::edit_render(Renderer& dc)
    {
        auto sze = _region.size();
        for (uint32_t n = 0; n < sze; n += 2)
        {
            Vec2f from(_region.get_item(n % sze).get(0.f), _region.get_item((n + 1) % sze).get(0.f));
            Vec2f to(_region.get_item((n + 2) % sze).get(0.f), _region.get_item((n + 3) % sze).get(0.f));
            dc.render_line(from, to);
        }
    }

} // namespace fin
