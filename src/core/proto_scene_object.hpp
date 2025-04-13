#pragma once

#include "atlas.hpp"
#include "atlas_scene_object.hpp"
#include "renderer.hpp"
#include "scene_object.hpp"
#include "prototype.hpp"

namespace fin
{
class ProtoSceneObject : public AtlasSceneObject
{
public:
    inline static std::string_view type_id = "prto";

    ProtoSceneObject() = default;
    virtual ~ProtoSceneObject() = default;

    void render(Renderer &dc) override;
    void serialize(msg::Writer &ar) override;
    void deserialize(msg::Value &ar) override;
    std::string_view object_type() override
    {
        return type_id;
    };
    void get_iso(Region<float> &bbox, Line<float> &origin) override;
    void begin_edit() override;
    void end_edit() override;
    bool edit() override;

    bool load_prototype(const msg::Var &ar);

private:
    msg::Var _data;
    Vec2f _isoa;
    Vec2f _isob;
};


inline void ProtoSceneObject::render(Renderer &dc)
{
    if (!_atlas || !_spr)
        return;

    Rectf dest;
    dest.x = _position.x - _spr->_origina.x;
    dest.y = _position.y - _spr->_origina.y;
    dest.width = _spr->_source.width;
    dest.height = _spr->_source.height;

    dc.render_texture(_spr->_texture,
                      _spr->_source, dest);
}

inline void ProtoSceneObject::serialize(msg::Writer &ar)
{
    SceneObject::serialize(ar);

    ar.member("proto", _data.get_item(ObjUid).get(0ull));
}

inline void ProtoSceneObject::deserialize(msg::Value &ar)
{
    SceneObject::deserialize(ar);

    auto pt = PrototypeRegister::instance().prototype(ar["proto"].get(0ull));
    load_prototype(pt);
}

inline void ProtoSceneObject::get_iso(Region<float> &bbox, Line<float> &origin)
{
    if (_spr)
    {
        bbox.x1 = _position.x - _spr->_origina.x;
        bbox.y1 = _position.y - _spr->_origina.y;
        bbox.x2 = bbox.x1 + _spr->_source.width;
        bbox.y2 = bbox.y1 + _spr->_source.height;
        origin.point1 = _position + _isoa;
        origin.point2 = _position + _isob;
    }
}

inline void ProtoSceneObject::begin_edit()
{
}

inline void ProtoSceneObject::end_edit()
{
}

inline bool ProtoSceneObject::load_prototype(const msg::Var &ar)
{
    bool ret = false;
    _data = ar;
    auto obj = _data.get_item(SceneObjId);
    if (obj.is_object())
    {
        auto spr = obj.get_item("spr");
        ret |= load_atlas(spr.get_item(0).str());
        ret |= load_sprite(spr.get_item(1).str());

        auto ia = obj.get_item("isoa");
        _isoa.x = ia.get_item(0).get(_isoa.x);
        _isoa.y = ia.get_item(1).get(_isoa.x);

        auto ib = obj.get_item("isob");
        _isob.x = ib.get_item(0).get(_isob.x);
        _isob.y = ib.get_item(1).get(_isob.x);
    }

    return ret;
}

} // namespace fin
