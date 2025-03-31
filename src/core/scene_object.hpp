#pragma once

#include "include.hpp"
#include "utils/lquery.hpp"
#include "utils/spatial.hpp"

namespace fin
{
class Renderer;

enum SceneObjectFlag
{
    Marked = 1 << 0,
    Disabled = 1 << 1,
    Hidden = 1 << 2,
};

class SceneObject : public lq::SpatialDatabase::Proxy
{
    friend class Scene;

public:
    inline static std::string_view type_id = "scno";

    SceneObject() = default;
    virtual ~SceneObject() = default;

    virtual void render(Renderer &dc) = 0;
    virtual void serialize(msg::Writer &ar);
    virtual void deserialize(msg::Value &ar);
    virtual void get_iso(Region<float> &bbox, Line<float> &origin) = 0;
    virtual std::string_view object_type()
    {
        return type_id;
    };


    bool flag_get(SceneObjectFlag f) const;
    void flag_reset(SceneObjectFlag f);
    void flag_set(SceneObjectFlag f);

    bool is_disabled() const
    {
        return flag_get(SceneObjectFlag::Disabled);
    }
    void disable(bool v)
    {
        v ? flag_set(SceneObjectFlag::Disabled) : flag_reset(SceneObjectFlag::Disabled);
    }

    bool is_hidden() const
    {
        return flag_get(SceneObjectFlag::Hidden);
    }
    void hide(bool v)
    {
        v ? flag_set(SceneObjectFlag::Hidden) : flag_reset(SceneObjectFlag::Hidden);
    }

protected:
    uint32_t _id{};
    uint32_t _flag{};
};


inline void SceneObject::serialize(msg::Writer &ar)
{
    ar.member("x", _position.x);
    ar.member("y", _position.y);
    ar.member("fl", _flag);
}

inline void SceneObject::deserialize(msg::Value &ar)
{
    _position.x = ar["x"].get(_position.x);
    _position.y = ar["y"].get(_position.y);
    _flag = ar["fl"].get(_flag);
}

inline bool SceneObject::flag_get(SceneObjectFlag f) const
{
    return _flag & f;
}

inline void SceneObject::flag_reset(SceneObjectFlag f)
{
    _flag &= ~f;
}

inline void SceneObject::flag_set(SceneObjectFlag f)
{
    _flag |= f;
}
} // namespace fin
