#include "scene_object.hpp"

#include "scene.hpp"

namespace fin
{

    SceneFactory* s_factory{};

    void SceneObject::serialize(msg::Writer& ar)
    {
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

    void SceneObject::attach(Scene* scene)
    {
        _scene = scene;
        _scene->object_insert(this);
    }

    void SceneObject::detach()
    {
        _scene->object_remove(this);
        _scene = nullptr;
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
            out.x2 = out.x1 + _img.sprite->_texture->width;
            out.y2 = out.y1 + _img.sprite->_texture->height;
        }
        return out;
    }

    void SceneObject::save(msg::Var& ar)
    {
        ar.set_item(Sc::Class, object_type());
        ar.set_item("x", _position.x);
        ar.set_item("y", _position.y);
        ar.set_item("fl", _flag);
        auto iso = msg::Var::array(4);
        iso.push_back(_iso.point1.x);
        iso.push_back(_iso.point1.y);
        iso.push_back(_iso.point2.x);
        iso.push_back(_iso.point2.y);
        ar.set_item("iso", iso);
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
        _position.x = ar["x"].get(_position.x);
        _position.y = ar["y"].get(_position.y);
        _flag       = ar["fl"].get(_flag);
        set_sprite(ar["atl"].str(), ar["spr"].str());
        auto iso      = ar.get_item("iso");
        _iso.point1.x = iso.get_item(0).get(0.f);
        _iso.point1.y = iso.get_item(1).get(0.f);
        _iso.point2.x = iso.get_item(2).get(0.f);
        _iso.point2.y = iso.get_item(3).get(0.f);
    }

    SceneObject* SceneObject::create(msg::Var& ar)
    {
        if (auto* obj = s_factory->create(ar[Sc::Class].str()))
        {
            obj->load(ar);
            return obj;
        }
        return nullptr;
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

    void SceneFactory::show_workspace()
    {
        if (!_edit || !_edit->is_selected())
            return;

        auto active = _edit->items.get_item(_edit->selected);
    }

    void SceneFactory::show_menu()
    {
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

                    if (ImGui::SmallButton(" " ICON_FA_LAPTOP " Add "))
                    {
                        auto obj = msg::Var::object(2);
                        obj.set_item(Sc::Uid, std::generate_unique_id());
                        obj.set_item(Sc::Class, el.first);
                        el.second.items.push_back(obj);
                        el.second.select(el.second.items.size() - 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" " ICON_FA_UPLOAD " Save "))
                    {
                        save_prototypes(el.first);
                    }
                    ImGui::SameLine();
                    ImGui::InvisibleButton("##ib", {-34, 1});
                    ImGui::SameLine();
                    if (ImGui::SmallButton(" " ICON_FA_BAN " "))
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
                                if (ImGui::Selectable(ImGui::FormatStr("%d", n), el.second.selected == n))
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

        info.obj->edit();
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

} // namespace fin
