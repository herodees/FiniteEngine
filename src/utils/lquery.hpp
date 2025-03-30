#pragma once

#include "math_utils.hpp"

#include <memory>

namespace fin
{
    namespace lq
    {
        class database
        {
        public:
            struct client_proxy
            {
                client_proxy*  _next{};
                client_proxy*  _prev{};
                client_proxy** _bin{};
                Vec2f          _position;
                uintptr_t      _data{};
            };

        public:
            database() = default;

            void           init(Rectf region, int32_t divx, int32_t divy);
            void           remove_from_bin(client_proxy* object);
            void           update_for_new_location(client_proxy* object);
            int            bin_index(float x, float y) const;
            client_proxy** bin_for_location(float x, float y);

            template <typename CB>
            void map_over_all_objects_in_locality(float x, float y, float radius, CB cb);
            template <typename CB>
            void map_over_all_objects_in_locality(const Rectf& rc, CB cb);

        private:
            void add_to_bin(client_proxy* object, client_proxy** bin);

            Rectf                            _region;
            int32_t                          _divx{};
            int32_t                          _divy{};
            std::unique_ptr<client_proxy*[]> _bins;
            client_proxy*                    _other{};
        };

        inline void database::init(Rectf region, int32_t divx, int32_t divy)
        {
            _region             = region;
            _divx               = divx;
            _divy               = divy;
            const auto bincount = _divx * _divy;
            _bins               = std::make_unique<client_proxy*[]>(bincount);
            _other              = nullptr;
        }

        inline int database::bin_index(float x, float y) const
        {
            /* if point inside super-brick, compute the bin coordinates */
            const auto ix = (int)(((x - _region.x) / _region.width) * _divx);
            const auto iy = (int)(((y - _region.y) / _region.height) * _divy);
            /* convert to linear bin number */
            return ((iy * _divx) + ix);
        }

        inline void database::remove_from_bin(client_proxy* object)
        {
            if (object->_bin != nullptr)
            {
                /* If this object is at the head of the list, move the bin
                   pointer to the next item in the list (might be NULL). */
                if (*(object->_bin) == object)
                    *(object->_bin) = object->_next;

                /* If there is a prev object, link its "next" pointer to the
                   object after this one. */
                if (object->_prev != nullptr)
                    object->_prev->_next = object->_next;

                /* If there is a next object, link its "prev" pointer to the
                   object before this one. */
                if (object->_next != nullptr)
                    object->_next->_prev = object->_prev;
            }
            object->_prev = nullptr;
            object->_next = nullptr;
            object->_bin  = nullptr;
        }

        inline void database::add_to_bin(client_proxy* object, client_proxy** bin)
        {
            /* if bin is currently empty */
            if (*bin == nullptr)
            {
                object->_prev = nullptr;
                object->_next = nullptr;
                *bin          = object;
            }
            else
            {
                object->_prev = nullptr;
                object->_next = *bin;
                (*bin)->_prev = object;
                *bin          = object;
            }

            /* record bin ID in proxy object */
            object->_bin = bin;
        }

        inline database::client_proxy** database::bin_for_location(float x, float y)
        {
            /* if point outside super-brick, return the "other" bin */
            if (!_region.contains(x, y))
                return &(_other);

            /* convert to linear bin number */
            const auto i = bin_index(x, y);

            /* return pointer to that bin */
            return &(_bins.get()[i]);
        }

        inline void database::update_for_new_location(client_proxy* object)
        {
            /* find bin for new location */
            client_proxy** newBin = bin_for_location(object->_position.x, object->_position.y);

            /* has object moved into a new bin? */
            if (newBin != object->_bin)
            {
                remove_from_bin(object);
                add_to_bin(object, newBin);
            }
        }

        template <typename CB>
        inline void traverse_bin_client_object_list(database::client_proxy* co, float x, float y, float radiusSquared, CB cb)
        {
            while (co != nullptr)
            {
                /* compute distance (squared) from this client   */
                /* object to given locality sphere's centerpoint */
                const float dx              = x - co->_position.x;
                const float dy              = y - co->_position.y;
                const float distanceSquared = (dx * dx) + (dy * dy);
                /* apply function if client object within sphere */
                if (distanceSquared < radiusSquared)
                    cb(co->_data, distanceSquared);
                /* consider next client object in bin list */
                co = co->_next;
            }
        }

        template <typename CB>
        inline void database::map_over_all_objects_in_locality(float x, float y, float radius, CB cb)
        {
            const bool completelyOutside = (((x + radius) < _region.x) || ((y + radius) < _region.y) ||
                                            ((x - radius) >= _region.x2()) || ((y - radius) >= _region.y2()));
            const auto radiusSqrt        = radius * radius;

            /* is the sphere completely outside the "super brick"? */
            if (completelyOutside)
            {
                traverse_bin_client_object_list<CB>(_other, x, y, radiusSqrt, cb);
                return;
            }

            /* compute min and max bin coordinates for each dimension */
            int  minBinX   = (int)((((x - radius) - _region.x) / _region.width) * _divx);
            int  minBinY   = (int)((((y - radius) - _region.y) / _region.height) * _divy);
            int  maxBinX   = (int)((((x + radius) - _region.x) / _region.width) * _divx);
            int  maxBinY   = (int)((((y + radius) - _region.y) / _region.height) * _divy);
            bool partlyOut = false;

            /* clip bin coordinates */
            if (minBinX < 0)
            {
                partlyOut = true;
                minBinX   = 0;
            }
            if (minBinY < 0)
            {
                partlyOut = true;
                minBinY   = 0;
            }
            if (maxBinX >= _divx)
            {
                partlyOut = true;
                maxBinX   = _divx - 1;
            }
            if (maxBinY >= _divy)
            {
                partlyOut = true;
                maxBinY   = _divy - 1;
            }

            /* map function over outside objects if necessary (if clipped) */
            if (partlyOut)
                traverse_bin_client_object_list<CB>(_other, x, y, radiusSqrt, cb);


            auto* bins = _bins.get();
            for (int ny = minBinY; ny <= maxBinY; ++ny)
            {
                const int line = ny * _divx;
                for (int nx = minBinX; nx <= maxBinX; ++nx)
                {
                    client_proxy* bin = bins[nx + line];
                    traverse_bin_client_object_list<CB>(bin, x, y, radiusSqrt, cb);
                }
            }
        }

        template <typename CB>
        inline void traverse_bin_client_object_list(database::client_proxy* co, const Rectf& rc, CB cb)
        {
            while (co != nullptr)
            {
                if (rc.contains(co->_position))
                    cb(co->_data);
                co = co->_next;
            }
        }

        template <typename CB>
        inline void database::map_over_all_objects_in_locality(const Rectf& rc, CB cb)
        {
            /* is the sphere completely outside the "super brick"? */
            if (!_region.intersects(rc))
            {
                traverse_bin_client_object_list<CB>(_other, rc, cb);
                return;
            }

            /* compute min and max bin coordinates for each dimension */
            int minBinX = (int)(((rc.x - _region.x) / _region.width) * _divx);
            int minBinY = (int)(((rc.y - _region.y) / _region.height) * _divy);
            int maxBinX = (int)(((rc.x2() - _region.x) / _region.width) * _divx);
            int maxBinY = (int)(((rc.y2() - _region.y) / _region.height) * _divy);

            bool partlyOut = false;

            /* clip bin coordinates */
            if (minBinX < 0)
            {
                partlyOut = true;
                minBinX   = 0;
            }
            if (minBinY < 0)
            {
                partlyOut = true;
                minBinY   = 0;
            }
            if (maxBinX >= _divx)
            {
                partlyOut = true;
                maxBinX   = _divx - 1;
            }
            if (maxBinY >= _divy)
            {
                partlyOut = true;
                maxBinY   = _divy - 1;
            }

            /* map function over outside objects if necessary (if clipped) */
            if (partlyOut)
                traverse_bin_client_object_list<CB>(_other, rc, cb);

            auto* bins = _bins.get();
            for (int ny = minBinY; ny <= maxBinY; ++ny)
            {
                const int line = ny * _divx;
                for (int nx = minBinX; nx <= maxBinX; ++nx)
                {
                    client_proxy* bin = bins[nx + line];
                    traverse_bin_client_object_list<CB>(bin, rc, cb);
                }
            }
        }
    }; // namespace lq
};     // namespace box
