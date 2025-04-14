#pragma once

#include "include.hpp"
#include "utils/lquery.hpp"
#include "utils/spatial.hpp"
#include "atlas.hpp"

namespace fin
{
class Renderer;
class Scene;

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

    virtual void update(float dt){};
    virtual void render(Renderer &dc) = 0;
    virtual void render_edit(Renderer &dc){};
    virtual void begin_edit(){};
    virtual void end_edit(){};
    virtual bool edit();
    virtual void serialize(msg::Writer &ar);
    virtual void deserialize(msg::Value &ar);
    virtual void get_iso(Region<float> &bbox, Line<float> &origin) = 0;
    virtual std::string_view object_type() { return type_id; };

    bool flag_get(SceneObjectFlag f) const;
    void flag_reset(SceneObjectFlag f);
    void flag_set(SceneObjectFlag f);

    bool is_disabled() const;
    void disable(bool v);

    bool is_hidden() const;
    void hide(bool v);

    Scene *scene();

    Vec2f position() const;
    void move(Vec2f pos);
    void move_to(Vec2f pos);

    void attach(Scene *scene);
    void detach();

    Atlas::Pack &set_sprite(std::string_view path, std::string_view spr);
    Atlas::Pack &sprite();
    Line<float> iso() const;
    Region<float> bounding_box() const;

    virtual void save(msg::Writer &ar);
    virtual void load(msg::Value &ar);

    static SceneObject *create(msg::Value &ar);

protected:
    uint32_t _id{};
    uint32_t _flag{};
    Scene *_scene{};
    Atlas::Pack _img;
    Line<float> _iso;
};


class SceneFactory
{
    using fact_t = SceneObject*(*)();
public:
    SceneFactory();
    ~SceneFactory() = default;
    static SceneFactory& instance();

    SceneObject *create(std::string_view type) const;
    template <class T>
    SceneFactory &factory(std::string_view type);

private:
    std::unordered_map<std::string, fact_t, std::string_hash, std::equal_to<>> _factory;
};



template <class T>
inline SceneFactory &SceneFactory::factory(std::string_view type)
{
    _factory[std::string(type)] = []() { return new T(); };
}

} // namespace fin
