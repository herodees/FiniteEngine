#include "scene_object.hpp"
#include "scene.hpp"

namespace fin
{

SceneFactory *s_factory{};

void SceneObject::serialize(msg::Writer &ar)
{
    ar.member("x", _position.x);
    ar.member("y", _position.y);
    ar.member("fl", _flag);
}

void SceneObject::deserialize(msg::Value &ar)
{
    _position.x = ar["x"].get(_position.x);
    _position.y = ar["y"].get(_position.y);
    _flag = ar["fl"].get(_flag);
}

void SceneObject::move_to(Vec2f pos)
{
    _position = pos;
    _scene->_spatial_db.update_for_new_location(this);
}

void SceneObject::attach(Scene *scene)
{
    _scene = scene;
    _scene->object_insert(this);
}

void SceneObject::detach()
{
    _scene->object_remove(this);
    _scene = nullptr;
}

Atlas::Pack &SceneObject::set_sprite(std::string_view path, std::string_view spr)
{
    _img = Atlas::load_shared(path, spr);
    return _img;
}

Atlas::Pack &SceneObject::sprite()
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

void SceneObject::save(msg::Writer &ar)
{
    ar.member("$cls", object_type());
    ar.member("x", _position.x);
    ar.member("y", _position.y);
    ar.member("fl", _flag);
    if (_img.atlas)
    {
        ar.member("atl", _img.atlas->get_path());
        if (_img.sprite)
        {
            ar.member("spr", _img.sprite->_name);
        }
    }
}

void SceneObject::load(msg::Value &ar)
{
    _position.x = ar["x"].get(_position.x);
    _position.y = ar["y"].get(_position.y);
    _flag = ar["fl"].get(_flag);
    set_sprite(ar["atl"].str(), ar["spr"].str());
}

SceneObject *SceneObject::create(msg::Value &ar)
{
    if (auto* obj = s_factory->create(ar["$cls"].str()))
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

Scene *SceneObject::scene()
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

SceneFactory &SceneFactory::instance()
{
    return *s_factory;
}

SceneObject *SceneFactory::create(std::string_view type) const
{
    auto it = _factory.find(type);
    if (it == _factory.end())
        return nullptr;
    return it->second();
}


} // namespace fin
