#pragma once

#include "math_utils.hpp"

#include <span>
#include <string>
#include <vector>

namespace fin
{
    template <typename T, typename BoundsGetter, int32_t MAX_OBJECTS = 4, int32_t MAX_LEVELS = 8>
    class LooseQuadTree
    {
        using ThisType = LooseQuadTree<T, BoundsGetter, MAX_OBJECTS, MAX_LEVELS>;

        struct ObjectSlot
        {
            ObjectSlot() = default;
            ~ObjectSlot()
            {
                if (!empty)
                    get()->~T();
            }

            std::aligned_storage_t<sizeof(T), alignof(T)> data;
            int16_t                                       next  = -1;
            bool                                          empty = true;

            T* get()
            {
                return std::launder(reinterpret_cast<T*>(&data));
            }
            const T* get() const
            {
                return std::launder(reinterpret_cast<const T*>(&data));
            }
        };

        struct Node
        {
            Rectf   bounds;
            int32_t children[4] = {-1, -1, -1, -1};
            int16_t firstObject = -1;
            int16_t level       = 0;
            int32_t objectCount = 0;

            bool hasChildren() const
            {
                return children[0] != -1;
            }
        };

        std::vector<Node>       nodes;
        std::vector<ObjectSlot> objects;
        std::vector<int32_t>    active;
        int32_t                 rootIndex          = -1;
        int32_t                 nodeFreeListHead   = -1;
        int32_t                 objectFreeListHead = -1;
        BoundsGetter            boundsGetter;

    public:
        LooseQuadTree(const Rectf& worldBounds, size_t size = 1024, size_t obj_size = 1024)
        {
            nodes.reserve(size);
            objects.reserve(obj_size);
            rootIndex = allocNode(worldBounds, 0);
        }
        LooseQuadTree(const LooseQuadTree&)             = delete;
        LooseQuadTree& operator=(const LooseQuadTree&)  = delete;
        LooseQuadTree(LooseQuadTree&& other)            = default;
        LooseQuadTree& operator=(LooseQuadTree&& other) = default;

        void swap(LooseQuadTree& other)
        {
            std::swap(nodes, other.nodes);
            std::swap(objects, other.objects);
            std::swap(active, other.active);
            std::swap(rootIndex, other.rootIndex);
            std::swap(nodeFreeListHead, other.nodeFreeListHead);
        }

        void clear()
        {
            auto worldBounds = nodes[rootIndex].bounds;
            objects.clear();
            nodes.clear();

            nodeFreeListHead   = -1;
            objectFreeListHead = -1;
            rootIndex          = allocNode(worldBounds, 0);
        }

        void resize(const Rectf& worldBounds)
        {
            if (nodes[rootIndex].bounds.contains(worldBounds))
                return;

            // Create a new tree with the new world bounds
            ThisType newTree(worldBounds);

            // Transfer all objects from the old tree to the new tree
            for (int32_t i = 0; i < objects.size(); ++i)
            {
                if (objects[i].empty)
                    continue;
                const T& obj = *objects[i].get();
                newTree.insert(obj); // Insert the object into the new tree
            }
            swap(newTree);
        }

        int32_t insert(const T& obj)
        {
            const Rectf objBounds = boundsGetter(obj);

            if (!nodes[rootIndex].bounds.contains(objBounds))
            {
                Rectf newBounds = getCombinedBounds(nodes[rootIndex].bounds, objBounds);
                resize(newBounds);
            }

            return insert(rootIndex, obj, objBounds);
        }

        void remove(const T& obj)
        {
            remove(rootIndex, obj, boundsGetter(obj));
        }

        int32_t find_at(float x, float y)
        {
            return find_at(rootIndex, x, y);
        }

        void query(const Rectf& area, std::vector<int32_t>& out) const
        {
            query(rootIndex, area, out);
        }

        void activate(const Rectf& area)
        {
            active.clear();
            query(rootIndex, area, active);
        }

        std::span<const int32_t> get_active() const
        {
            return active;
        }

        bool is_empty(int32_t n) const
        {
            return objects[n].empty;
        }

        T& operator[](int32_t n)
        {
            return *objects[n].get();
        }

        const T& operator[](int32_t n) const
        {
            return objects[n].object;
        }

        int32_t size() const
        {
            return (int32_t)objects.size();
        }

        template <typename CB>
        void for_each(CB cb)
        {
            for (size_t n = 0; n < objects.size(); ++n)
            {
                auto& obj = objects[n];
                if (!obj.empty)
                    cb(*obj.get());
            }
        }

        template <typename CB>
        void for_each_active(CB cb)
        {
            for (auto n : active)
            {
                auto& obj = objects[n];
                if (!obj.empty)
                    cb(obj.object);
            }
        }

        template <typename CB>
        void sort_active(CB cb)
        {
            std::sort(active.begin(), active.end(), cb);
        }

        template <typename CB>
        void for_each_node(CB cb)
        {
            for_each_node<CB>(rootIndex, cb);
        }

    private:
        Rectf getCombinedBounds(const Rectf& a, const Rectf& b)
        {
            float minX = std::min(a.x, b.x);
            float maxX = std::max(a.x2(), b.x2());
            float minY = std::min(a.y, b.y);
            float maxY = std::max(a.y2(), b.y2());

            return Rectf{minX, minY, maxX - minX, maxY - minY};
        }

        template <typename CB>
        void for_each_node(int32_t nodeIdx, CB cb)
        {
            if (nodeIdx == -1)
                return;

            const auto& node = nodes[nodeIdx];
            cb(node.bounds);

            for (int32_t i = 0; i < 4; ++i)
            {
                if (node.children[i] == -1)
                    continue;
                for_each_node<CB>(node.children[i], cb);
            }
        }

        int32_t allocNode(const Rectf& bounds, int16_t level)
        {
            if (nodeFreeListHead != -1)
            {
                int idx          = nodeFreeListHead;
                nodeFreeListHead = nodes[idx].children[0]; // abuse children[0] as freelist ptr
                nodes[idx]       = Node{bounds, {-1, -1, -1, -1}, -1, level};
                return idx;
            }
            else
            {
                nodes.push_back(Node{bounds, {-1, -1, -1, -1}, -1, level});
                return static_cast<int>(nodes.size()) - 1;
            }
        }

        void freeNode(int idx)
        {
            nodes[idx].firstObject  = -1;
            nodes[idx].objectsCount = 0;
            nodes[idx].bounds       = {0, 0, 0, 0};
            nodes[idx].level        = -1;
            nodes[idx].children[0]  = nodeFreeListHead;
            nodes[idx].children[1] = nodes[idx].children[2] = nodes[idx].children[3] = -1;
            nodeFreeListHead                                                         = idx;
        }

        int32_t allocObject(const T& obj)
        {
            int32_t idx{};
            if (objectFreeListHead != -1)
            {
                idx                = objectFreeListHead;
                objectFreeListHead = objects[idx].next;
            }
            else
            {
                idx = static_cast<int32_t>(objects.size());
                objects.emplace_back();
            }
            objects[idx].empty = false;
            new (objects[idx].get()) T(obj); // placement new
            return idx;
        }

        void freeObject(int32_t idx)
        {
            objects[idx].get()->~T(); // manually destroy
            objects[idx].next  = objectFreeListHead;
            objects[idx].empty = true;
            objectFreeListHead = idx;
        }

        int32_t find_at(int32_t nodeIdx, float x, float y)
        {
            const Node& node = nodes[nodeIdx];

            if (!node.bounds.contains({x, y}))
                return -1;

            int32_t current = node.firstObject;
            while (current != -1)
            {
                const Rectf& objBounds = boundsGetter(*objects[current].get());
                if (objBounds.contains({x, y}))
                    return current;
                current = objects[current].next;
            }

            if (node.hasChildren())
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    int32_t childIdx = node.children[i];
                    if (childIdx != -1)
                    {
                        int32_t result = find_at(childIdx, x, y);
                        if (result != -1)
                            return result;
                    }
                }
            }

            return -1;
        }

        void split(int32_t nodeIdx)
        {
            Node& node = nodes[nodeIdx];
            float subW = node.bounds.width / 2.0f;
            float subH = node.bounds.height / 2.0f;
            float x    = node.bounds.x;
            float y    = node.bounds.y;

            // Define the bounds of the child nodes
            Rectf subRects[4] = {
                Rectf(x, y, subW, subH),               // top-left
                Rectf(x + subW, y, subW, subH),        // top-right
                Rectf(x, y + subH, subW, subH),        // bottom-left
                Rectf(x + subW, y + subH, subW, subH), // bottom-right
            };

            // Allocate child nodes
            for (int32_t i = 0; i < 4; ++i)
            {
                node.children[i] = allocNode(subRects[i], node.level + 1);
            }

            // Now redistribute objects to their correct child nodes
            int32_t current  = node.firstObject;
            node.firstObject = -1; // Clear the current list of objects in this node

            while (current != -1)
            {
                int32_t next      = objects[current].next;
                Rectf   objBounds = boundsGetter(*objects[current].get()); // Get bounds of the object

                // Find which child node this object belongs to
                int32_t idx = getChildIndex(nodeIdx, objBounds);
                if (idx != -1) // If the object belongs to a child node
                {
                    // Insert the object into the appropriate child
                    insert(node.children[idx], *objects[current].get(), objBounds);
                    freeObject(current); // No longer needed in the parent node
                }
                else // If the object doesn't fit any of the children, leave it in the parent
                {
                    objects[current].next = node.firstObject;
                    node.firstObject      = current;
                }
                current = next;
            }
        }

        Rectf getObjectBounds(int32_t nodeIdx) const
        {
            return boundsGetter(*objects[nodeIdx].get());
        }

        int32_t getChildIndex(int32_t nodeIdx, const Rectf& objBounds) const
        {
            const Node& node = nodes[nodeIdx];

            for (int32_t i = 0; i < 4; ++i)
            {
                int32_t childIdx = node.children[i];
                if (childIdx == -1)
                    continue;

                const Rectf& looseBounds = nodes[childIdx].bounds;

                if (looseBounds.contains(objBounds))
                    return i;
            }

            return -1;
        }

        int32_t insert(int32_t nodeIdx, const T& obj, const Rectf& bounds)
        {
            Node& node = nodes[nodeIdx];

            // If node has children, insert into the appropriate child node
            if (node.hasChildren())
            {
                int32_t idx = getChildIndex(nodeIdx, bounds);
                if (idx != -1)
                {
                    return insert(node.children[idx], obj, bounds); // Recursive insert into the child;
                }
            }

            // Add the object to the current node's list
            int32_t objIdx       = allocObject(obj);
            objects[objIdx].next = node.firstObject;
            node.firstObject     = objIdx;
            ++node.objectCount;

            // If the number of objects exceeds the max limit and the node hasn't reached the max level
            if (node.objectCount > MAX_OBJECTS && node.level < MAX_LEVELS)
            {
                // Split the node and redistribute the objects
                if (!node.hasChildren()) // Split the node only if it hasn't been split already
                    split(nodeIdx);
            }

            return objIdx;
        }

        bool remove(int32_t nodeIdx, const T& obj, const Rectf& bounds)
        {
            Node& node = nodes[nodeIdx];
            if (!node.bounds.intersects(bounds))
                return false;

            int16_t* prev    = &node.firstObject;
            int16_t  current = node.firstObject;

            while (current != -1)
            {
                if (*objects[current].get() == obj)
                {
                    *prev = objects[current].next;
                    freeObject(current);
                    --node.objectCount;
                    return true;
                }
                prev    = &objects[current].next;
                current = objects[current].next;
            }

            if (node.hasChildren())
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    if (node.children[i] != -1)
                    {
                        if (remove(node.children[i], obj, bounds))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        void query(int32_t nodeIdx, const Rectf& area, std::vector<int32_t>& out) const
        {
            const Node& node = nodes[nodeIdx];
            if (!node.bounds.intersects(area))
                return;

            int32_t current = node.firstObject;
            while (current != -1)
            {
                if (area.intersects(getObjectBounds(current)))
                {
                    out.push_back(current);
                }
                current = objects[current].next;
            }

            if (node.hasChildren())
            {
                for (int32_t i = 0; i < 4; ++i)
                {
                    if (node.children[i] != -1)
                        query(node.children[i], area, out);
                }
            }
        }
    };


} // namespace fin
