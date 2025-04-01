#pragma once

#include "atlas.hpp"
#include "scene_object.hpp"
#include "renderer.hpp"

namespace fin
{
    class AtlasSceneObject : public SceneObject
    {
    public:
        inline static std::string_view type_id = "atlo";

        AtlasSceneObject() = default;
        virtual ~AtlasSceneObject() = default;

        void render(Renderer& dc) override;
        void serialize(msg::Writer& ar) override;
        void deserialize(msg::Value& ar) override;
        std::string_view object_type() override { return type_id; };
        void get_iso(Region<float> &bbox, Line<float> &origin) override;

        void set_atlas(std::shared_ptr<Atlas>& atlas);
        void set_sprite(Atlas::sprite* spr);
        bool load_atlas(std::string_view path);
        bool load_sprite(std::string_view path);

        Atlas *get_atlas();
        Atlas::sprite *get_sprite();

    protected:
        std::shared_ptr<Atlas> _atlas;
        Atlas::sprite* _spr{};
    };



    inline void AtlasSceneObject::render(Renderer &dc)
    {
        if (!_atlas || !_spr)
            return;

        dc.render_texture(_spr->_texture,
                          _spr->_source,
                          {_position.x, _position.y, _spr->_source.width, _spr->_source.height});
    }

    inline void AtlasSceneObject::set_atlas(std::shared_ptr<Atlas>& atlas)
    {
        _atlas = atlas;
        set_sprite(nullptr);
    }

    inline void AtlasSceneObject::set_sprite(Atlas::sprite* spr)
    {
        _spr = spr;
    }

    inline bool AtlasSceneObject::load_atlas(std::string_view path)
    {
        _atlas = Atlas::load_shared(path);
        set_sprite(nullptr);
        return _atlas.get();
    }

    inline bool AtlasSceneObject::load_sprite(std::string_view path)
    {
        if (!_atlas) {
            set_sprite(nullptr);
            return false;
        }

        if (auto n = _atlas->find_sprite(path))
        {
            set_sprite(& _atlas->get(n));
        }
        return !!_spr;
    }

    inline Atlas *AtlasSceneObject::get_atlas()
    {
        return _atlas.get();
    }

    inline Atlas::sprite *AtlasSceneObject::get_sprite()
    {
        return _spr;
    }

    inline void AtlasSceneObject::serialize(msg::Writer& ar)
    {
        SceneObject::serialize(ar);
        if (_atlas && _spr)
        {
            ar.member("src", _atlas.get()->get_path());
            ar.member("spr", _spr->_name);
        }
    }

    inline void AtlasSceneObject::deserialize(msg::Value& ar)
    {
        SceneObject::deserialize(ar);
        auto path = ar["src"].get(std::string_view());
        if (!path.empty())
        {
            _atlas = Atlas::load_shared(path);
            load_sprite(ar["spr"].get(std::string_view()));
        }

    }

    inline void AtlasSceneObject::get_iso(Region<float> &bbox, Line<float> &origin)
    {
        if (_spr)
        {
            bbox.x1 = _position.x;
            bbox.y1 = _position.y;
            bbox.x2 = _position.x + _spr->_source.width;
            bbox.y2 = _position.y + _spr->_source.height;
            origin.point1 = _position + _spr->_origina;
            origin.point2 = _position + _spr->_originb;
        }
    }

    }
