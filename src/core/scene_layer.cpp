#include "scene_layer.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "scene_object.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "editor/imgui_control.hpp"

namespace fin
{
    constexpr int32_t tile_size(512);
    std::string       _buffer;

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

        void edit_active()
        {
            if (_edit)
            {
                _buffer = _edit->is_named() ? _edit->_name : "";
                if (ImGui::InputText("Name", &_buffer))
                {
                    parent()->name_object(_edit, _buffer);
                }

                _edit->edit_update();
            }
        }

        void edit_update(bool items) override
        {
            if (!items)
                return edit_active();

            if (ImGui::Button(" " ICON_FA_BAN " "))
            {
                if (_edit)
                {
                    destroy(_edit);
                    _edit = nullptr;
                }
            }

            if (ImGui::BeginChildFrame(-1, {-1, 250}, 0))
            {
                ImGuiListClipper clipper;
                clipper.Begin(_scene.size());
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        ImGui::PushID(n);
                        auto*       el   = _scene[n];
                        const char* name = el->_name;
                        if (!name)
                            name = ImGui::FormatStr("Object %p", el);

                        if (ImGui::Selectable(name, el == _edit))
                        {
                            select(el);
                        }
                        ImGui::PopID();
                    }
                }
            }

            ImGui::EndChildFrame();
        }

        void select(IsoSceneObject* obj)
        {
            if (_edit != obj)
            {
                _edit = obj;
            }
        }

        void insert(IsoSceneObject* obj)
        {
            if (!obj)
                return;
            obj->_id = _scene.size();
            _scene.push_back(obj);
            _spatial_db.update_for_new_location(obj);
        }

        void remove(IsoSceneObject* obj)
        {
            if (obj->is_named())
            {
                parent()->name_object(obj, {});
            }
            _spatial_db.remove_from_bin(obj);
            const auto id   = obj->_id;
            _scene[id]      = _scene.back();
            _scene[id]->_id = id;
            _scene.pop_back();
        }

        void destroy(IsoSceneObject* obj)
        {
            if (obj)
            {
                remove(obj);
            }
            delete obj;
        }

        void moveto(IsoSceneObject* obj, Vec2f pos)
        {
            obj->_position = pos;
            _spatial_db.update_for_new_location(obj);
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
                    _spatial.insert(nde);
                }
            }
        }

        void destroy(int32_t n)
        {

        }

        void edit_active()
        {

        }

        void edit_update(bool items) override
        {
            if (!items)
                return edit_active();

            if (ImGui::Button(" " ICON_FA_BAN " "))
            {
                if (_edit)
                {
                    destroy(_edit);
                    _edit = -1;
                }
            }

            if (ImGui::BeginChildFrame(-1, {-1, 250}, 0))
            {
                ImGuiListClipper clipper;

                clipper.Begin(_spatial.size());
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        ImGui::PushID(n);
                        auto& el = _spatial[n];
                        if (el._sprite.sprite)
                        {
                            if (ImGui::Selectable(el._sprite.sprite->_name.c_str(), n == _edit))
                            {
                                _edit = n;
                            }
                        }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChildFrame();
        }
    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        int32_t                                                                              _edit = -1;
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
                _spatial.insert(nde);
            }
        }

    private:
        LooseQuadTree<Node, decltype([](const Node& n) -> const Rectf& { return n._bbox; })> _spatial;
        int32_t                                                                              _edit = -1;
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

    Scene* SceneLayer::parent()
    {
        return _parent;
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

    void SceneLayer::edit_update(bool items)
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

    void SceneLayer::hide(bool b)
    {
        _hidden = b;
    }

    void SceneLayer::activate(bool a)
    {
        _active = a;
    }


}
