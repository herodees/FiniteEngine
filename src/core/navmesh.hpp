#pragma once

#include "include.hpp"
#include "shared_resource.hpp"

namespace fin
{
    enum TerrainFlags : uint8_t
    {
        TERRAIN_BLOCKED = 1 << 0, // bit 0
        TERRAIN_SLOW    = 1 << 1, // bit 1
        TERRAIN_FAST    = 1 << 2, // bit 2
        TERRAIN_DANGER  = 1 << 3  // bit 3
    };

    class Navmesh
    {
        struct Node
        {
            int   x = 0, y = 0;
            float gCost   = std::numeric_limits<float>::infinity();
            float fCost   = std::numeric_limits<float>::infinity();
            int   parentX = 0, parentY = 0;
            bool  open = false, closed = false;
        };

    public:
        Navmesh(float worldWidth, float worldHeight, int gridCellWidth, int gridCellHeight);
        ~Navmesh();

        void resize(float worldWidth, float worldHeight, int gridCellWidth, int gridCellHeight);
        bool findPath(Vec2i start, Vec2i end, std::vector<Vec2i>& outPath) const;
        bool isWalkable(int x, int y) const;
        bool lineOfSight(const Vec2i& start, const Vec2i& end) const;

        Vec2i          worldToCell(Vec2f pos) const;
        Vec2f          cellToWorld(Vec2i cell) const;
        Vec2f          worldSize() const;
        Vec2i          cellSize() const;
        void           applyTerrain(const std::vector<Vec2f>& poly, uint8_t flag, bool add);
        void           resetTerrain();
        const Texture& getDebugTexture() const;

    private:
        float cost(int x, int y) const;
        void  reconstructPath(const Vec2i&                           endPos,
                              const std::unordered_map<Vec2i, Node>& nodes,
                              std::vector<Vec2i>&                    outPath) const;
        void  refinePath(std::vector<Vec2i>& path) const;
        void  raycastOptimize(std::vector<Vec2i>& path) const;

        std::vector<uint8_t> terrain;
        Vec2f                size;
        Vec2i                cell;
        Vec2i                cellsize;
        mutable bool         changed{};
        mutable Texture      debug{};
    };
} // namespace fin
