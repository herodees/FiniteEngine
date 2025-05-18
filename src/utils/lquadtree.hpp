#pragma once

#include "math_utils.hpp"
#include <string>
#include <vector>
#include <span>

namespace fin
{
    template <typename T, typename BoundsGetter, int MAX_OBJECTS = 4, int MAX_LEVELS = 5, float LOOSE_FACTOR = 2.0f>
    class LooseQuadTree
    {
        using ThisType = LooseQuadTree<T, BoundsGetter, MAX_OBJECTS, MAX_LEVELS, LOOSE_FACTOR>;

        struct ObjectEntry
        {
            T   object;
            int next = -1;
            bool empty = true;
        };

        struct Node
        {
            Rectf bounds;
            int  children[4] = {-1, -1, -1, -1};
            int  firstObject = -1;
            int  level       = 0;

            bool hasChildren() const
            {
                return children[0] != -1;
            }
        };

        std::vector<Node>        nodes;
        std::vector<ObjectEntry> objects;
        std::vector<int>         active;
        int rootIndex          = -1;
        int nodeFreeListHead   = -1;
        int objectFreeListHead = -1;
        BoundsGetter boundsGetter;

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
            nodes.clear();
            objects.clear();
            nodeFreeListHead   = -1;
            objectFreeListHead = -1;
            rootIndex          = allocNode(worldBounds, 0);
        }

        void resize(const Rectf& worldBounds)
        {
            // Create a new tree with the new world bounds
            ThisType newTree(worldBounds);

            // Transfer all objects from the old tree to the new tree
            for (int i = 0; i < objects.size(); ++i)
            {
                if (objects[i].empty)
                    continue;
                const T& obj = objects[i].object;
                newTree.insert(obj);             // Insert the object into the new tree
            }
            swap(newTree);
        }

        int insert(const T& obj)
        {
            return insert(rootIndex, obj, boundsGetter(obj));
        }

        void remove(const T& obj)
        {
            remove(rootIndex, obj, boundsGetter(obj));
        }

        int find_at(float x, float y)
        {
            return find_at(rootIndex, x, y);
        }

        void query(const Rectf& area, std::vector<int>& out) const
        {
            query(rootIndex, area, out);
        }

        void activate(const Rectf& area)
        {
            active.clear();
            query(rootIndex, area, active);
        }

        std::span<const int> get_active() const
        {
            return active;
        }

        bool is_empty(int n) const
        {
            return objects[n].empty;
        }

        T& operator[](int n)
        {
            return objects[n].object;
        }

        const T& operator[](int n) const
        {
            return objects[n].object;
        }

        int size() const
        {
            return (int)objects.size();
        }

        template <typename CB>
        void for_each(CB cb)
        {
            for (size_t n = 0; n < objects.size(); ++n)
            {
                auto& obj = objects[n];
                if (!obj.empty)
                    cb(obj.object);
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
    private:
        int allocNode(const Rectf& bounds, int level)
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
            nodes[idx].firstObject = -1;
            nodes[idx].bounds      = {0, 0, 0, 0};
            nodes[idx].level       = -1;
            nodes[idx].children[0] = nodeFreeListHead;
            nodes[idx].children[1] = nodes[idx].children[2] = nodes[idx].children[3] = -1;
            nodeFreeListHead                                                         = idx;
        }

        int allocObject(const T& obj)
        {
            if (objectFreeListHead != -1)
            {
                int idx            = objectFreeListHead;
                objectFreeListHead = objects[idx].next;
                objects[idx]       = ObjectEntry{obj, -1, 0};
                return idx;
            }
            else
            {
                objects.push_back(ObjectEntry{obj, -1, 0});
                return static_cast<int>(objects.size()) - 1;
            }
        }

        void freeObject(int idx)
        {
            objects[idx].empty = 1;
            objects[idx].next  = objectFreeListHead;
            objectFreeListHead = idx;
        }

        int find_at(int nodeIdx, float x, float y)
        {
            const Node& node = nodes[nodeIdx];

            if (!node.bounds.contains({x, y}))
                return -1;

            int current = node.firstObject;
            while (current != -1)
            {
                const Rectf& objBounds = boundsGetter(objects[current].object);
                if (objBounds.contains({x, y}))
                    return current;
                current = objects[current].next;
            }

            if (node.hasChildren())
            {
                for (int i = 0; i < 4; ++i)
                {
                    int childIdx = node.children[i];
                    if (childIdx != -1)
                    {
                        int result = find_at(childIdx, x, y);
                        if (result != -1)
                            return result;
                    }
                }
            }

            return -1;
        }

        void split(int nodeIdx)
        {
            Node& node = nodes[nodeIdx];
            float subW = node.bounds.width / 2.0f;
            float subH = node.bounds.height / 2.0f;
            float x    = node.bounds.x;
            float y    = node.bounds.y;

            // Define the bounds of the child nodes
            Rectf subRects[4] = {
                {x - subW / 2, y - subH / 2, subW * LOOSE_FACTOR, subH * LOOSE_FACTOR},
                {x + subW / 2, y - subH / 2, subW * LOOSE_FACTOR, subH * LOOSE_FACTOR},
                {x - subW / 2, y + subH / 2, subW * LOOSE_FACTOR, subH * LOOSE_FACTOR},
                {x + subW / 2, y + subH / 2, subW * LOOSE_FACTOR, subH * LOOSE_FACTOR},
            };

            // Allocate child nodes
            for (int i = 0; i < 4; ++i)
            {
                node.children[i] = allocNode(subRects[i], node.level + 1);
            }

            // Now redistribute objects to their correct child nodes
            int current      = node.firstObject;
            node.firstObject = -1; // Clear the current list of objects in this node

            while (current != -1)
            {
                int   next      = objects[current].next;
                Rectf objBounds = boundsGetter(objects[current].object); // Get bounds of the object

                // Find which child node this object belongs to
                int idx = getChildIndex(nodeIdx, objBounds);
                if (idx != -1) // If the object belongs to a child node
                {
                    // Insert the object into the appropriate child
                    insert(node.children[idx], objects[current].object, objBounds);
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

        Rectf getObjectBounds(int nodeIdx) const
        {
            return boundsGetter(objects[nodeIdx].object);
        }

        int getChildIndex(int nodeIdx, const Rectf& objBounds) const
        {
            const Node& node = nodes[nodeIdx];

            // For each of the 4 child nodes, check if the object fits within the child node bounds
            for (int i = 0; i < 4; ++i)
            {
                int childIdx = node.children[i];
                if (childIdx != -1 && nodes[childIdx].bounds.contains(objBounds))
                {
                    return i; // Return the child index if the object fits
                }
            }

            return -1; // If no suitable child is found, return -1 (could happen if object doesn't fit)
        }

        int insert(int nodeIdx, const T& obj, const Rectf& bounds)
        {
            Node& node = nodes[nodeIdx];

            // If node has children, insert into the appropriate child node
            if (node.hasChildren())
            {
                int idx = getChildIndex(nodeIdx, bounds);
                if (idx != -1)
                {
                    return insert(node.children[idx], obj, bounds); // Recursive insert into the child;
                }
            }

            // Add the object to the current node's list
            int objIdx           = allocObject(obj);
            objects[objIdx].next = node.firstObject;
            node.firstObject     = objIdx;

            // If the number of objects exceeds the max limit and the node hasn't reached the max level
            if (countObjects(node) > MAX_OBJECTS && node.level < MAX_LEVELS)
            {
                // Split the node and redistribute the objects
                if (!node.hasChildren()) // Split the node only if it hasn't been split already
                    split(nodeIdx);
            }

            return objIdx;
        }

        bool remove(int nodeIdx, const T& obj, const Rectf& bounds)
        {
            Node& node = nodes[nodeIdx];
            if (!node.bounds.intersects(bounds))
                return false;

            int* prev    = &node.firstObject;
            int  current = node.firstObject;

            while (current != -1)
            {
                if (objects[current].object == obj)
                {
                    *prev = objects[current].next;
                    freeObject(current);
                    return true;
                }
                prev    = &objects[current].next;
                current = objects[current].next;
            }

            if (node.hasChildren())
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (node.children[i] != -1)
                    {
                        if (remove(node.children[i], obj, bounds))
                        {
                            Node& child = nodes[node.children[i]];
                            if (child.firstObject == -1 && !child.hasChildren())
                            {
                                freeNode(node.children[i]);
                                node.children[i] = -1;
                            }
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        void query(int nodeIdx, const Rectf& area, std::vector<int>& out) const
        {
            const Node& node = nodes[nodeIdx];
            if (!node.bounds.intersects(area))
                return;

            int current = node.firstObject;
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
                for (int i = 0; i < 4; ++i)
                {
                    if (node.children[i] != -1)
                        query(node.children[i], area, out);
                }
            }
        }

        int countObjects(const Node& node) const
        {
            int count   = 0;
            int current = node.firstObject;
            while (current != -1)
            {
                ++count;
                current = objects[current].next;
            }
            return count;
        }
    };


} // namespace fin
