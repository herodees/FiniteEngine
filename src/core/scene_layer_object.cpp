#include "scene_layer.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "application.hpp"
#include "scene_object.hpp"
#include "utils/lquadtree.hpp"
#include "utils/lquery.hpp"
#include "utils/imguiline.hpp"
#include "editor/imgui_control.hpp"

#include "core/ecs/base.hpp"

namespace fin
{
    int32_t ObjectSceneLayer::IsoObject::depth_get()
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

    Region<float> ObjectSceneLayer::IsoObject::bbox_get()
    {
        return Region<float>();
    }

    Line<float> ObjectSceneLayer::IsoObject::iso_get()
    {
        return Line<float>();
    }

    ObjectSceneLayer::ObjectSceneLayer() : SceneLayer(SceneLayer::Type::Object)
    {
        name() = "ObjectLayer";
        icon() = ICON_FA_MAP_PIN;
        _color = 0xffa0a0ff;
    };

    ObjectSceneLayer ::~ObjectSceneLayer()
    {
        clear();
    }

    void ObjectSceneLayer::clear()
    {
        _iso_pool.clear();
        _iso.clear();
        _grid_size     = {};
        _iso_pool_size = {};
        _objects.clear();
    }

    void ObjectSceneLayer::resize(Vec2f size)
    {
        _grid_size.x = (size.width + (tile_size - 1)) / tile_size; // Round up division
        _grid_size.y = (size.height + (tile_size - 1)) / tile_size;
        _spatial_db.init({0, 0, (float)_grid_size.x * tile_size, (float)_grid_size.y * tile_size},
                         _grid_size.x,
                         _grid_size.y);

        for (Entity et : _objects)
        {
            if (ecs::Base::contains(et))
            {
                auto* obj = ecs::Base::get(et);
                obj->_bin   = nullptr;
                obj->_prev  = nullptr;
                obj->_next  = nullptr;
                _spatial_db.update_for_new_location(obj);
            }
        }
    }

    void ObjectSceneLayer::activate(const Rectf& region)
    {
        SceneLayer::activate(region);

        _selected.clear();
        _iso_pool_size = 0;
        auto cb = [&](lq::SpatialDatabase::Proxy* item)
        {
            if (_iso_pool_size >= _iso_pool.size())
                _iso_pool.resize(_iso_pool_size + 64);

            _iso_pool[_iso_pool_size]._ptr = static_cast<ecs::Base*>(item)->_self;
            ++_iso_pool_size;
        };

        auto add_pool_item = [&](size_t n)
        {
            _iso[n]                    = &_iso_pool[n];
            _iso_pool[n]._depth        = 0;
            _iso_pool[n]._depth_active = false;
            _iso_pool[n]._back.clear();
            _iso_pool[n]._bbox   = _iso_pool[n].bbox_get();
            _iso_pool[n]._origin = _iso_pool[n].iso_get();
            _selected.emplace(_iso_pool[n]._ptr);
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
        if (_edit != entt::null)
        {
            _iso_pool[_iso_pool_size]._ptr = _edit;
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

} // namespace fin
