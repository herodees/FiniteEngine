#include "scene_object.hpp"

#include "scene.hpp"
#include "editor/imgui_control.hpp"
#include "utils/imguiline.hpp"
#include "application.hpp"

namespace fin
{
    struct FileDir
    {
        std::string                    path;
        std::vector<std::string>       dirs;
        std::vector<std::string>       files;
        std::shared_ptr<ImGui::Editor> editor;
        bool                           expanded{};
    };

    SceneFactory* s_factory{};
    FileDir       s_file_dir{};

    void SpriteSceneObject::serialize(msg::Writer& ar)
    {
        IsoSceneObject::serialize(ar);
    }

    void SpriteSceneObject::deserialize(msg::Value& ar)
    {
        IsoSceneObject::deserialize(ar);
    }

    bool SpriteSceneObject::sprite_object() const
    {
        return true;
    }

    Atlas::Pack& SpriteSceneObject::set_sprite(std::string_view path, std::string_view spr)
    {
        _img = Atlas::load_shared(path, spr);
        return _img;
    }

    Atlas::Pack& SpriteSceneObject::sprite()
    {
        return _img;
    }

    Region<float> SpriteSceneObject::bounding_box() const
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

    void SpriteSceneObject::render(Renderer& dc)
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

    void SpriteSceneObject::edit_render(Renderer& dc, bool selected)
    {
        IsoSceneObject::edit_render(dc, selected);

        if (!_img.sprite)
            return;

       
    }

    bool SpriteSceneObject::imgui_update()
    {
        bool modified = IsoSceneObject::imgui_update();

        modified |= ImGui::SpriteInput("Sprite", &_img);
        modified |= ImGui::PointVector("Collision", &_collision, {-1, ImGui::GetFrameHeightWithSpacing() * 4});

        return modified;
    }

    void SpriteSceneObject::save(msg::Var& ar)
    {
        IsoSceneObject::save(ar);
       
        if (_img.atlas)
        {
            ar.set_item("atl", _img.atlas->get_path());
            if (_img.sprite)
            {
                ar.set_item("spr", _img.sprite->_name);
            }
        }
    }

    void SpriteSceneObject::load(msg::Var& ar)
    {
        IsoSceneObject::load(ar);

        set_sprite(ar["atl"].str(), ar["spr"].str());
    }



    SceneFactory::SceneFactory()
    {
        s_factory = this;
    }

    SceneFactory& SceneFactory::instance()
    {
        return *s_factory;
    }

    IsoSceneObject* SceneFactory::create(std::string_view type) const
    {
        auto it = _factory.find(type);
        if (it == _factory.end())
            return nullptr;
        return it->second.fn();
    }

    IsoSceneObject* SceneFactory::create(uint64_t uid) const
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
        s_file_dir.expanded = false;
        s_file_dir.path     = startPath;
    }

    void SceneFactory::center_view()
    {
        _scroll_to_center = true;
    }

    void SceneFactory::imgui_workspace()
    {
        if (!_edit || !_edit->is_selected())
            return;

        if (!_edit->obj->sprite_object())
            return;

        static int                 s_drag_point = 0;
        static ImVec2              s_prev_mpos;
        static std::vector<ImVec2> s_points;
        if (!ImGui::IsMouseDown(0))
        {
            s_drag_point = 0;
        }

        SpriteSceneObject* obj = static_cast<SpriteSceneObject*>(_edit->obj.get());
        auto               spr = obj->sprite();
        Params      params;

        ImVec2 visible_size = ImGui::GetContentRegionAvail();
        params.pos          = ImGui::GetWindowPos();
        auto cur            = ImGui::GetCursorPos();
        params.dc           = ImGui::GetWindowDrawList();
        auto mpos           = ImGui::GetMousePos();

        params.scroll = {ImGui::GetScrollX(), ImGui::GetScrollY()};
        params.pos.x -= ImGui::GetScrollX();
        params.pos.y -= ImGui::GetScrollY();

        ImGui::InvisibleButton("Canvas", ImVec2(2000, 2000));
        auto itemid = ImGui::GetItemID();
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
                ImVec2 a{params.pos.x + obj->_iso.point1.x, params.pos.y + obj->_iso.point1.y};
                ImVec2 b{params.pos.x + obj->_iso.point2.x, params.pos.y + obj->_iso.point2.y};

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
                    auto& iso   = s_drag_point == 1 ? obj->_iso.point1 : obj->_iso.point2;
                    iso -= diff;
                }

                params.dc->AddLine(a, b, 0xff909090, 2);
                hovera ? params.dc->AddCircleFilled(a, 5, 0xff00ff00) : params.dc->AddCircle(a, 5, 0xff00ff00);
                hoverb ? params.dc->AddCircleFilled(b, 5, 0xff0000ff) : params.dc->AddCircle(b, 5, 0xff0000ff);
            }

            if (_edit_collision && obj->collision().is_array())
            {
                auto coll = obj->collision();
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

        ImGui::ScrollWhenDragging({-1, -1}, ImGuiMouseButton_Right, itemid);
    }

    void SceneFactory::imgui_workspace_menu()
    {
        if (ImGui::LineItem(ImGui::GetID("wspf"), {-1, ImGui::GetFrameHeightWithSpacing()})
                .Space()
                .PushStyle(ImStyle_Button, 1, (!_edit_collision && !_edit_origin))
                .Text(ICON_FA_ARROW_POINTER " Select")
                .PopStyle()
                .Space()
                .PushStyle(ImStyle_Button, 2, _edit_origin)
                .Text(ICON_FA_CROSSHAIRS " Origin")
                .PopStyle()
                .Space()
                .PushStyle(ImStyle_Button, 3, _edit_collision)
                .Text(ICON_FA_VECTOR_SQUARE " Collision")
                .PopStyle()
                .Space()
                .End())
        {
            if (ImGui::Line().HoverId() == 1)
            {
                _edit_origin    = false;
                _edit_collision = false;
            }
            if (ImGui::Line().HoverId() == 2)
            {
                _edit_origin    = true;
                _edit_collision = false;
            }
            if (ImGui::Line().HoverId() == 3)
            {
                _edit_origin    = false;
                _edit_collision = true;
            }
        }
    }

    void SceneFactory::imgui_properties()
    {
        if (ImGui::BeginTabBar("ScnFactTab"))
        {
            _edit = nullptr;
            for (auto* cls : _classes)
            {
                if (ImGui::BeginTabItem(cls->name.c_str()))
                {
                    _edit = cls;

                    if (ImGui::Button(" " ICON_FA_LAPTOP " Add "))
                    {
                        auto obj = msg::Var::object(2);
                        obj.set_item(Sc::Uid, std::generate_unique_id());
                        obj.set_item(Sc::Class, cls->id);
                        obj.set_item(Sc::Id, ImGui::FormatStr("%s_%d", cls->id.data(), cls->items.size()));
                        cls->items.push_back(obj);
                        cls->select(cls->items.size() - 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(" " ICON_FA_UPLOAD " Save "))
                    {
                        cls->select(cls->selected);
                        save_prototypes(cls->id);
                    }
                    ImGui::SameLine();
                    ImGui::InvisibleButton("##ib", {-34, 1});
                    ImGui::SameLine();
                    if (ImGui::Button(" " ICON_FA_BAN " "))
                    {
                        if ((uint32_t)cls->selected < cls->items.size())
                        {
                            auto sel = cls->selected;
                            cls->items.erase(sel);
                            cls->select(sel);
                        }
                    }

                    if (ImGui::BeginChildFrame(-1, {-1, 100}))
                    {
                        ImGuiListClipper clipper;
                        clipper.Begin(cls->items.size());
                        while (clipper.Step())
                        {
                            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                            {
                                ImGui::PushID(n);
                                auto val = cls->items.get_item(n);
                                auto id = val.get_item(Sc::Id);
                                if (ImGui::Selectable(id.c_str(), cls->selected == n))
                                {
                                    cls->select(n);
                                }
                                ImGui::PopID();
                            }
                        }
                    }
                    ImGui::EndChildFrame();
                    ImGui::Text("Properties");
                    if (ImGui::BeginChildFrame(-2, {-1, -1}))
                    {
                        show_properties(*cls);
                    }
                    ImGui::EndChildFrame();

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    void SceneFactory::imgui_explorer()
    {
        if (!ImGui::Begin("Explorer"))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("ExpFactTab"))
        {
            ImGui::SetNextItemWidth(90);
            if (ImGui::BeginTabItem("Assets"))
            {
                imgui_file_explorer();
                ImGui::EndTabItem();
            }

            ImGui::SetNextItemWidth(90);
            if (ImGui::BeginTabItem("Prefabs"))
            {
                if (ImGui::BeginTabBar("PrefabTab"))
                {
                    for (auto* cls : _classes)
                    {
                        auto& info = *cls;
                        if (ImGui::BeginTabItem(info.name.c_str()))
                        {
                            if (_explore != &info)
                            {
                                _explore = &info;
                                reset_atlas_cache();
                            }

                            if (ImGui::BeginChildFrame(-1, {-1, -1}))
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
                                            ImGui::SetDragData(info.obj.get(), "PREFAB");
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

                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();

        ImGui::End();
    }

    inline std::string_view get_file_icon(std::string_view file, uint32_t& clr)
    {
        clr = 0xff808080;
        auto p = file.rfind(".");
        if (p == std::string_view::npos)
            return ICON_FA_FILE_PEN;
        auto ext = file.substr(p + 1);
        if (ext == "ogg" || ext == "wav")
        {
            clr = 0xff0051f2;
            return ICON_FA_FILE_AUDIO;
        }
        if (ext == "png" || ext == "jpg" || ext == "gif")
        {
            clr = 0xff60d71e;
            return ICON_FA_FILE_IMAGE;
        }
        if (ext == "atlas")
        {
            clr = 0xffcc6f98;
            return ICON_FA_FILE_ZIPPER;
        }
        if (ext == "prefab")
        {
            clr = 0xfff0f0f0;
            return ICON_FA_FILE_CODE;
        }
        return ICON_FA_FILE;
    }

    void SceneFactory::imgui_file_explorer()
    {
        if (!s_file_dir.expanded)
        {
            s_file_dir.expanded = true;
            s_file_dir.dirs.clear();
            s_file_dir.files.clear();
            if (s_file_dir.path != _base_folder)
            {
                s_file_dir.dirs.push_back("..");
            }

            for (const auto& entry : std::filesystem::directory_iterator(s_file_dir.path))
            {
                if (entry.is_directory())
                {
                    s_file_dir.dirs.push_back(entry.path().filename().string());
                }
                else if (entry.is_regular_file())
                {
                    s_file_dir.files.push_back(entry.path().filename().string());
                }
            }
        }


        if (ImGui::BeginChildFrame(-1, { -1, -1 }))
        {
            if (s_file_dir.editor)
            {
                if (ImGui::LineSelect(ImGui::GetID(".."), false)
                        .Space()
                        .PushColor(0xff52d1ff)
                        .Text(ICON_FA_FOLDER)
                        .PopColor()
                        .Space()
                        .Text("..")
                        .End() ||
                    !s_file_dir.editor->imgui_show())
                {
                    s_file_dir.editor.reset();
                }
            }
            else
            {
                for (auto& dir : s_file_dir.dirs)
                {
                    if (ImGui::LineSelect(ImGui::GetID(dir.c_str()), false)
                            .Space()
                            .PushColor(0xff52d1ff)
                            .Text(ICON_FA_FOLDER)
                            .PopColor()
                            .Space()
                            .Text(dir.c_str())
                            .End())
                    {
                        s_file_dir.expanded = false;
                        if (dir == "..")
                        {
                            s_file_dir.path.pop_back();
                            s_file_dir.path.resize(s_file_dir.path.rfind('/') + 1);
                        }
                        else
                        {
                            s_file_dir.path += dir;
                            s_file_dir.path += '/';
                        }
                    }
                }

                for (auto& file : s_file_dir.files)
                {
                    uint32_t clr;
                    auto     ico = get_file_icon(file, clr);

                    if (ImGui::LineSelect(ImGui::GetID(file.c_str()), false)
                            .Space()
                            .PushColor(clr)
                            .Text(ico)
                            .PopColor()
                            .Space()
                            .Text(file.c_str())
                            .End())
                    {
                        s_file_dir.editor = ImGui::Editor::load_from_file(s_file_dir.path + file);
                    }
                }
            }
        }
        ImGui::EndChildFrame();
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

        info.obj->imgui_update();
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

    bool ObjectBase::flag_get(SceneObjectFlag f) const
    {
        return _flag & f;
    }

    void ObjectBase::flag_reset(SceneObjectFlag f)
    {
        _flag &= ~f;
    }

    void ObjectBase::flag_set(SceneObjectFlag f)
    {
        _flag |= f;
    }
    bool ObjectBase::is_disabled() const
    {
        return flag_get(SceneObjectFlag::Disabled);
    }

    void ObjectBase::disable(bool v)
    {
        v ? flag_set(SceneObjectFlag::Disabled) : flag_reset(SceneObjectFlag::Disabled);
    }

    bool ObjectBase::is_hidden() const
    {
        return flag_get(SceneObjectFlag::Hidden);
    }

    void ObjectBase::hide(bool v)
    {
        v ? flag_set(SceneObjectFlag::Hidden) : flag_reset(SceneObjectFlag::Hidden);
    }

    bool ObjectBase::is_named() const
    {
        return _name;
    }

    bool ObjectBase::is_region() const
    {
        return flag_get(SceneObjectFlag::Area);
    }


    std::vector<Vec2i> IsoSceneObject::find_path(Vec2i target) const
    {
        std::vector<Vec2i> out;
        _layer->find_path(this, target, out);

        return std::move(out);
    }

    Vec2f IsoSceneObject::position() const
    {
        return _position;
    }

    uint64_t IsoSceneObject::prefab() const
    {
        return _prefab_uid;
    }

    bool IsoSceneObject::imgui_update()
    {
        bool modified = false;

        modified |= ImGui::CheckboxFlags("Disabled", &_flag, SceneObjectFlag::Disabled);
        modified |= ImGui::CheckboxFlags("Hidden", &_flag, SceneObjectFlag::Hidden);

        return modified;
    }

    void IsoSceneObject::serialize(msg::Writer& ar)
    {
        ar.member(Sc::Uid, _prefab_uid);
        ar.member(Sc::Flag, _flag);
        ar.member("x", _position.x);
        ar.member("y", _position.y);
    }

    void IsoSceneObject::deserialize(msg::Value& ar)
    {
        _flag       = ar[Sc::Flag].get(_flag);
        _position.x = ar["x"].get(_position.x);
        _position.y = ar["y"].get(_position.y);
    }

    bool IsoSceneObject::sprite_object() const
    {
        return false;
    }

    SoundObject::SoundObject()
    {
    }

    SoundObject::~SoundObject()
    {
        set_sound("");
    }

    void SoundObject::update(float dt)
    {
        PlaySound(_alias);
    }

    void SoundObject::render(Renderer& dc)
    {
    }

    void SoundObject::edit_render(Renderer& dc, bool selected)
    {
        dc.set_color(WHITE);
        dc.render_line_circle(_position, _radius);
        
        if (selected)
        {
            dc.set_color(RED);
            dc.render_line_rect(bounding_box().rect());
        }
    }

    bool SoundObject::imgui_update()
    {
        auto ret = IsoSceneObject::imgui_update();

        ret |= ImGui::DragFloat("Radius", &_radius, 1.f, 0, 1000.f);
        if (ImGui::SoundInput("Sound", &_sound))
        {
            set_sound(_sound);
        }
        return ret;
    }

    void SoundObject::save(msg::Var& ar)
    {
        IsoSceneObject::save(ar);
        if (_radius)
        {
            ar.set_item("rad", _radius);
        }
        if (_sound)
        {
            ar.set_item("snd", _sound->get_path());
        }
    }

    void SoundObject::load(msg::Var& ar)
    {
        IsoSceneObject::load(ar);
        _radius = ar.get_item("rad").get(0.f);
        set_sound(ar.get_item("snd").str());
    }

    void SoundObject::serialize(msg::Writer& ar)
    {
        IsoSceneObject::serialize(ar);
        ar.member("rad", _radius);
    }

    void SoundObject::deserialize(msg::Value& ar)
    {
        IsoSceneObject::deserialize(ar);
        _radius = ar["rad"].get(_radius);
    }

    Sound& SoundObject::set_sound(std::string_view path)
    {
        UnloadSoundAlias(_alias);
        _alias              = {};
        _sound.reset();
        if (!path.empty())
        {
            _sound = SoundSource::load_shared(path);
            _alias = LoadSoundAlias(*_sound->get_sound());
        }
        return _alias;
    }

    Sound& SoundObject::set_sound(SoundSource::Ptr sound)
    {
        UnloadSoundAlias(_alias);
        _alias = {};
        _sound = sound;
        if (_sound)
        {
            _alias = LoadSoundAlias(*_sound->get_sound());
        }
        return _alias;
    }

    Sound& SoundObject::sound()
    {
        return _alias;
    }

    std::string_view SoundObject::object_type() const
    {
        return SoundObject::type_id;
    }

    Region<float> SoundObject::bounding_box() const
    {
        float rad = _radius;
        if (!rad)
        {
            rad = 5;
        }
        return Region<float>(_position.x - rad, _position.y - rad, _position.x + rad, _position.y + rad);
    }

    void IsoSceneObject::save(msg::Var& ar)
    {
        ar.set_item(Sc::Uid, _prefab_uid);
        ar.set_item(Sc::Class, object_type());
        ar.set_item(Sc::Flag, _flag);

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
    }

    void IsoSceneObject::load(msg::Var& ar)
    {
        _prefab_uid   = ar[Sc::Uid].get(0ull);
        _flag         = ar[Sc::Flag].get(0);

        _collision    = ar["coll"].clone();
        auto iso      = ar.get_item("iso");
        _iso.point1.x = iso.get_item(0).get(0.f);
        _iso.point1.y = iso.get_item(1).get(0.f);
        _iso.point2.x = iso.get_item(2).get(0.f);
        _iso.point2.y = iso.get_item(3).get(0.f);
    }

    void IsoSceneObject::edit_render(Renderer& dc, bool selected)
    {
        if (selected)
        {
            dc.set_color(RED);
            dc.render_line_rect(bounding_box().rect());
        }

        if (g_settings.visible_isometric)
        {
            dc.set_color({255, 255, 255, 255});
            auto line = iso();
            dc.render_line(line.point1, line.point2);
        }

        if (g_settings.visible_collision)
        {
            dc.set_color({255, 255, 0, 255});
            auto& coll = collision();
            auto  sze  = coll.size();
            if (coll.is_array() && sze >= 4)
            {
                auto p = position();
                for (uint32_t n = 0; n < sze; n += 2)
                {
                    Vec2f from(coll.get_item(n % sze).get(0.f), coll.get_item((n + 1) % sze).get(0.f));
                    Vec2f to(coll.get_item((n + 2) % sze).get(0.f), coll.get_item((n + 3) % sze).get(0.f));
                    dc.render_line(from + p, to + p);
                }
            }
        }
    }

    Line<float> IsoSceneObject::iso() const
    {
        return Line<float>(_iso.point1 + _position, _iso.point2 + _position);
    }

    msg::Var& IsoSceneObject::collision()
    {
        return _collision;
    }

    SceneLayer* IsoSceneObject::layer()
    {
        return _layer;
    }

} // namespace fin
