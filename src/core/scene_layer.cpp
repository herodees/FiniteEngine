#include "scene_layer.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "scene_object.hpp"
#include "renderer.hpp"

namespace fin
{
    constexpr int32_t tile_size(512);


    class IsometricSceneLayer : public SceneLayer
    {
        struct IsoObject
        {
            int32_t                 _depth        : 31;
            bool                    _depth_active : 1;
            Line<float>             _origin;
            Region<float>           _bbox;
            IsoSceneObject*         _ptr;
            std::vector<IsoObject*> _back;
            int32_t                 depth_get()
            {
                if (_depth_active)
                    return 0;

                if (!_depth)
                {
                    _depth_active = true;
                    if (_back.empty())
                        _depth = 1;
                    else
                    {
                        for (auto el : _back)
                            _depth = std::max(el->depth_get(), _depth);
                        _depth += 1;
                    }
                    _depth_active = false;
                }
                return _depth;
            }
        };

    public:
        IsometricSceneLayer() : SceneLayer(SceneLayer::Type::Isometric)
        {
            name() = "IsometricLayer";
            icon() = ICON_FA_MAP_PIN;
        };

        void resize(Vec2f size) override
        {
            _grid_size.x = (size.width + (tile_size - 1)) / tile_size; // Round up division
            _grid_size.y = (size.height + (tile_size - 1)) / tile_size;
            _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                             _grid_size.x,
                             _grid_size.y);

            for (auto* s : _scene)
            {
                s->_bin = nullptr;
                s->_prev = nullptr;
                s->_next = nullptr;

                _spatial_db.update_for_new_location(s);
            }
        }

        void activate(const Rectf& region) override
        {
            _iso_pool_size = 0;

            auto cb = [&](lq::SpatialDatabase::Proxy* item)
            {
                if (_iso_pool_size >= _iso_pool.size())
                    _iso_pool.resize(_iso_pool_size + 32);

                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(item);
                ++_iso_pool_size;
            };

            auto add_pool_item = [&](size_t n)
            {
                auto& obj                  = *_iso_pool[n]._ptr;
                _iso[n]                    = &_iso_pool[n];
                _iso_pool[n]._depth        = 0;
                _iso_pool[n]._depth_active = false;
                _iso_pool[n]._back.clear();
                _iso_pool[n]._bbox   = obj.bounding_box();
                _iso_pool[n]._origin = obj.iso();
                obj.flag_reset(SceneObjectFlag::Marked);
            };


            // Query active region
            _spatial_db.map_over_all_objects_in_locality({(float)region.x - tile_size,
                                                 (float)region.y - tile_size,
                                                 (float)region.width + 2 * tile_size,
                                                 (float)region.height + 2 * tile_size},
                                                cb);


            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 32);

            // Reset iso depth and sort data
            _iso.resize(_iso_pool_size);
            for (size_t n = 0; n < _iso_pool_size; ++n)
            {
                add_pool_item(n);
            }

            // Add edit object to render queue
            if (_edit)
            {
                _iso_pool[_iso_pool_size]._ptr = static_cast<IsoSceneObject*>(_edit);
                _iso.emplace_back();
                add_pool_item(_iso_pool_size);
            }


            // Topology sort
            if (_iso.size() > 1)
            {
                // Determine depth relationships
                for (size_t i = 0; i < _iso.size(); ++i)
                {
                    auto* a = _iso[i];
                    for (size_t j = i + 1; j < _iso.size(); ++j)
                    {
                        auto* b = _iso[j];
                        // Ignore non overlaped rectangles
                        if (a->_bbox.intersects(b->_bbox))
                        {
                            // Check if x2 is above iso line
                            if (b->_origin.compare(a->_origin) >= 0)
                            {
                                a->_back.push_back(b);
                            }
                            else
                            {
                                b->_back.push_back(a);
                            }
                        }
                    }
                }

                // Recursive function to calculate depth
                for (auto* obj : _iso)
                    obj->depth_get();

                // Sort objects by depth
                std::sort(_iso.begin(),
                          _iso.end(),
                          [](const IsoObject* a, const IsoObject* b) { return a->_depth < b->_depth; });
            }
        }

        void update(float dt) override
        {
            for (auto* obj : _iso)
            {
                if (!obj->_ptr->is_disabled())
                    obj->_ptr->update(dt);
            }
        }

        void render(Renderer& dc) override
        {
            for (auto* obj : _iso)
            {
                if (!obj->_ptr->is_hidden())
                    obj->_ptr->render(dc);
            }
        }

        void serialize(msg::Writer& ar)
        {
            SceneLayer::serialize(ar);
        }

        void deserialize(msg::Value& ar)
        {
            SceneLayer::deserialize(ar);
        }

    private:
        Vec2i                        _grid_size;
        lq::SpatialDatabase          _spatial_db;
        std::vector<IsoSceneObject*> _scene;
        std::vector<IsoObject>       _iso_pool;
        std::vector<IsoObject*>      _iso;
        uint32_t                     _iso_pool_size{};
        IsoSceneObject*              _edit{};
    };



    class SpriteSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            Atlas::Pack _sprite;
            Rectf       _bbox;
        };

        SpriteSceneLayer() : SceneLayer(SceneLayer::Type::Sprite), _spatial({})
        {
            name() = "SpriteLayer";
            icon() = ICON_FA_IMAGE;
        };

        void resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void activate(const Rectf& region) override
        {
            _spatial.activate(region);
        }

        void update(float dt) override
        {
        }

        void render(Renderer& dc) override
        {
            for (auto n : _spatial.get_active())
            {
                auto& nde = _spatial[n];
                dc.render_texture(nde._sprite.sprite->_texture, nde._sprite.sprite->_source, nde._bbox);
            }
        }
        void serialize(msg::Writer& ar)
        {
            SceneLayer::serialize(ar);

            auto save = [&ar](Node& node)
            {
                ar.begin_object();
                ar.member("a", node._sprite.atlas->get_path());
                ar.member("s", node._sprite.sprite->_name);
                ar.member("x", node._bbox.x);
                ar.member("y", node._bbox.y);
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
                nde._sprite = Atlas::load_shared(ar["a"].str(), ar["s"].str());
                if (nde._sprite.sprite)
                {
                    nde._bbox.x      = ar["x"].get(0.f);
                    nde._bbox.y      = ar["y"].get(0.f);
                    nde._bbox.width = nde._sprite.sprite->_source.width;
                    nde._bbox.height = nde._sprite.sprite->_source.height;
                    _spatial.insert(nde, nde._bbox);
                }
            }
        }

    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
    };



    class RegionSceneLayer : public SceneLayer
    {
    public:
        struct Node
        {
            msg::Var _points;
            Rectf    _bbox;
        };
        RegionSceneLayer() : SceneLayer(SceneLayer::Type::Region), _spatial({})
        {
            name() = "RegionLayer";
            icon() = ICON_FA_MAP_LOCATION_DOT;
        };

        void resize(Vec2f size) override
        {
            _spatial.resize({0, 0, size.width, size.height});
        }

        void activate(const Rectf& region) override
        {
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
                _spatial.insert(nde, nde._bbox);
            }
        }

    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
    };



    SceneLayer* SceneLayer::create(msg::Value& ar)
    {
        if (auto* obj = create(Type(ar["type"].get(0))))
        {
            obj->deserialize(ar);
            return obj;
        }
        return nullptr;
    }

    SceneLayer* SceneLayer::create(Type t)
    {
        switch (t)
        {
            case Type::Sprite:
                return new SpriteSceneLayer;
            case Type::Region:
                return new RegionSceneLayer;
            case Type::Isometric:
                return new IsometricSceneLayer;
        }
        return nullptr;
    }

    SceneLayer::SceneLayer(Type t) : _type(t)
    {
    }

    SceneLayer::Type SceneLayer::type() const
    {
        return _type;
    }

    std::string& SceneLayer::name()
    {
        return _name;
    }

    std::string_view& SceneLayer::icon()
    {
        return _icon;
    }

    void SceneLayer::serialize(msg::Writer& ar)
    {
        ar.member("type", (int)_type);
        ar.member("name", _name);
    }

    void SceneLayer::deserialize(msg::Value& ar)
    {
        _type = Type(ar["type"].get(0));
        _name = ar["name"].str();
    }

    void SceneLayer::resize(Vec2f size)
    {
    }

    void SceneLayer::activate(const Rectf& region)
    {
    }

    void SceneLayer::update(float dt)
    {
    }

    void SceneLayer::render(Renderer& dc)
    {
    }

    bool SceneLayer::is_hidden() const
    {
        return _hidden;
    }

    bool SceneLayer::is_active() const
    {
        return _active;
    }

    bool SceneLayer::is_edit() const
    {
        return _edit;
    }

    void SceneLayer::hide(bool b)
    {
        _hidden = b;
    }

    void SceneLayer::activate(bool a)
    {
        _active = a;
    }

    void SceneLayer::edit(bool a)
    {
        _edit = a;
    }

}
