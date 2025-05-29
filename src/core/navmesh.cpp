#include "navmesh.hpp"

namespace fin
{
    float heuristic(Vec2i a, Vec2i b)
    {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);
        return 1.0f * (dx + dy) + (1.4142f - 2) * std::min(dx, dy);
    }


    Navmesh::Navmesh(float worldWidth, float worldHeight, int gridCellWidth, int gridCellHeight) :
    cell(gridCellWidth, gridCellHeight)
    {
        resize(worldWidth, worldHeight, gridCellWidth, gridCellHeight);
    }

    Navmesh::~Navmesh()
    {
        UnloadTexture(debug);
    }

    void Navmesh::resize(float worldWidth, float worldHeight, int gridCellWidth, int gridCellHeight)
    {
        cell       = {gridCellWidth, gridCellHeight};
        size       = {worldWidth, worldHeight};
        cellsize.x = static_cast<int>(std::ceil(worldWidth / cell.x));
        cellsize.y = static_cast<int>(std::ceil(worldHeight / cell.y));
        terrain.resize(cellsize.x * cellsize.y);
        changed = true;
    }

    Vec2i Navmesh::worldToCell(Vec2f pos) const
    {
        return Vec2i{static_cast<int>(pos.x / cell.x), static_cast<int>(pos.y / cell.y)};
    }

    Vec2f Navmesh::cellToWorld(Vec2i cell) const
    {
        return Vec2f{(cell.x + 0.5f) * cell.x, (cell.y + 0.5f) * cell.y};
    }

    Vec2f Navmesh::worldSize() const
    {
        return size;
    }

    Vec2i Navmesh::cellSize() const
    {
        return cell;
    }

    bool Navmesh::isWalkable(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= cellsize.x || y >= cellsize.y)
            return false;
        return !(terrain[y * cellsize.x + x] & TERRAIN_BLOCKED);
    }

    float Navmesh::cost(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= cellsize.x || y >= cellsize.y)
            return std::numeric_limits<float>::infinity();

        uint8_t flags = terrain[y * cellsize.x + x];

        if (flags & TERRAIN_BLOCKED)
            return std::numeric_limits<float>::infinity();

        float base = 1.0f;

        if (flags & TERRAIN_SLOW)
            base *= 3.0f;
        if (flags & TERRAIN_FAST)
            base *= 0.5f;
        if (flags & TERRAIN_DANGER)
            base += 10.0f;

        return base;
    }

    void Navmesh::reconstructPath(const Vec2i&                           endPos,
                                  const std::unordered_map<Vec2i, Node>& nodes,
                                  std::vector<Vec2i>&                    outPath) const
    {
        outPath.clear();
        Vec2i currentPos = endPos;

        while (true)
        {
            outPath.push_back(currentPos);
            const Node& node = nodes.at(currentPos);

            if (node.parentX == -1 && node.parentY == -1)
                break;

            currentPos = {node.parentX, node.parentY};
        }

        std::reverse(outPath.begin(), outPath.end());
    }

    void Navmesh::refinePath(std::vector<Vec2i>& path) const
    {
        if (path.size() < 3)
            return; // nothing to refine

        size_t writeIdx = 1; // position to write the next kept point

        Vec2i lastDir = {path[1].x - path[0].x, path[1].y - path[0].y};
        if (lastDir.x != 0)
            lastDir.x /= std::abs(lastDir.x);
        if (lastDir.y != 0)
            lastDir.y /= std::abs(lastDir.y);

        for (size_t i = 2; i < path.size(); ++i)
        {
            Vec2i dir = {path[i].x - path[i - 1].x, path[i].y - path[i - 1].y};
            if (dir.x != 0)
                dir.x /= std::abs(dir.x);
            if (dir.y != 0)
                dir.y /= std::abs(dir.y);

            if (dir.x != lastDir.x || dir.y != lastDir.y)
            {
                path[writeIdx++] = path[i - 1]; // keep last turning point
                lastDir          = dir;
            }
        }

        // Always keep the last point (goal)
        path[writeIdx++] = path.back();

        // Resize to remove trimmed points
        path.resize(writeIdx);
    }

    void Navmesh::raycastOptimize(std::vector<Vec2i>& path) const
    {
        if (path.size() < 3)
            return;

        std::vector<Vec2i> optimized;
        optimized.push_back(path.front());

        size_t startIdx = 0;
        size_t nextIdx  = 1;

        while (nextIdx < path.size())
        {
            // Try to extend line of sight as far as possible
            size_t farthest = nextIdx;
            for (size_t i = nextIdx + 1; i < path.size(); ++i)
            {
                if (!lineOfSight(path[startIdx], path[i]))
                    break;
                farthest = i;
            }

            optimized.push_back(path[farthest]);
            startIdx = farthest;
            nextIdx  = startIdx + 1;
        }

        path = std::move(optimized);
    }
    // Returns true if the line from 'start' to 'end' is walkable (no obstacles)
    bool Navmesh::lineOfSight(const Vec2i& start, const Vec2i& end) const
    {
        int x0 = start.x, y0 = start.y;
        int x1 = end.x, y1 = end.y;

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int err = dx - dy;

        while (true)
        {
            if (!isWalkable(x0, y0))
                return false;

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }

        return true;
    }

    bool Navmesh::findPath(Vec2i start, Vec2i end, std::vector<Vec2i>& outPath) const
    {
        static constexpr int dx[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
        static constexpr int dy[8] = {0, 0, -1, 1, -1, 1, -1, 1};

        if (start == end)
        {
            outPath = {start};
            return true;
        }

        auto dist = start.distance_squared(end);
        if (dist < 10000 && lineOfSight(start, end))
        {
            outPath = {start, end};
            return true;
        }

        std::unordered_map<Vec2i, Node> nodes;

        auto& startNode = nodes[start];
        startNode.x     = start.x;
        startNode.y     = start.y;
        startNode.gCost = 0;
        startNode.fCost = heuristic(start, end);
        startNode.open  = true;

        auto cmp = [&](const Vec2i& a, const Vec2i& b) { return nodes[a].fCost > nodes[b].fCost; };

        using PQ = std::priority_queue<Vec2i, std::vector<Vec2i>, decltype(cmp)>;
        PQ open(cmp);
        open.push(start);

        Vec2i best     = {-1, -1};
        float bestDist = std::numeric_limits<float>::max();

        while (!open.empty())
        {
            Vec2i currentPos = open.top();
            open.pop();
            Node& current  = nodes[currentPos];
            current.closed = true;

            float distToGoal = heuristic(currentPos, end);
            if (distToGoal < bestDist)
            {
                bestDist = distToGoal;
                best     = currentPos;
            }

            if (currentPos == end)
            {
                reconstructPath(current, nodes, outPath);
                refinePath(outPath);
                return true;
            }

            for (int d = 0; d < 8; ++d)
            {
                int   nx          = current.x + dx[d];
                int   ny          = current.y + dy[d];
                Vec2i neighborPos = {nx, ny};

                if (!isWalkable(nx, ny))
                    continue;

                Node& neighbor = nodes[neighborPos];
                if (neighbor.closed)
                    continue;

                float moveCost = current.gCost + cost(nx, ny) * (d < 4 ? 1.0f : 1.4142f);
                if (!neighbor.open || moveCost < neighbor.gCost)
                {
                    neighbor.x       = nx;
                    neighbor.y       = ny;
                    neighbor.gCost   = moveCost;
                    neighbor.fCost   = moveCost + heuristic(neighborPos, end);
                    neighbor.parentX = current.x;
                    neighbor.parentY = current.y;

                    if (!neighbor.open)
                    {
                        neighbor.open = true;
                        open.push(neighborPos);
                    }
                }
            }
        }

        if (best.x != -1 && best.y != -1)
        {
            reconstructPath(nodes[best], nodes, outPath);
            refinePath(outPath);
            return true;
        }

        return false;
    }

    void Navmesh::applyTerrain(const std::vector<Vec2f>& poly, uint8_t flag, bool add)
    {
        if (poly.size() < 3)
            return;

        changed    = true;
        float minY = poly[0].y, maxY = poly[0].y;
        for (const auto& p : poly)
        {
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }

        int minRow = static_cast<int>(minY / cell.y);
        int maxRow = static_cast<int>(maxY / cell.y);

        for (int row = minRow; row <= maxRow; ++row)
        {
            float              scanY = row * cell.y + 0.5f * cell.y;
            std::vector<float> nodesX;

            for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
            {
                Vec2f p1 = poly[i], p2 = poly[j];
                if ((p1.y <= scanY && p2.y > scanY) || (p2.y <= scanY && p1.y > scanY))
                {
                    float x = p1.x + (scanY - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                    nodesX.push_back(x);
                }
            }

            std::sort(nodesX.begin(), nodesX.end());
            for (size_t i = 0; i + 1 < nodesX.size(); i += 2)
            {
                int startCol = static_cast<int>(nodesX[i] / cell.x);
                int endCol   = static_cast<int>(nodesX[i + 1] / cell.x);
                for (int col = startCol; col <= endCol; ++col)
                {
                    if (col >= 0 && col < cellsize.x && row >= 0 && row < cellsize.y)
                    {
                        uint8_t& cell = terrain[row * cellsize.x + col];
                        if (add)
                            cell |= flag;
                        else
                            cell &= ~flag;
                    }
                }
            }
        }
    }

    void Navmesh::resetTerrain()
    {
        std::fill(terrain.begin(), terrain.end(), 0);
    }

    const Texture& Navmesh::getDebugTexture() const
    {
        if (!changed)
            return debug;

        changed     = false;
        auto image = GenImageColor(cellsize.x, cellsize.y, {0, 0, 0, 0});
        for (int y = 0; y < cellsize.y; ++y)
        {
            for (int x = 0; x < cellsize.x; ++x)
            {
                uint8_t flags = terrain[y * cellsize.x + x];
                if (flags & TERRAIN_BLOCKED)
                {
                    Color color = {0, 0, 0, 255};
                    color = ColorAlpha(RED, 1.0f);
                    if (flags & TERRAIN_FAST)
                        color.g += 200;
                    if (flags & TERRAIN_SLOW)
                        color.b += 200;
                    if (flags & TERRAIN_DANGER)
                        color.r += 200;
                    ImageDrawPixel(&image, x, y, color);
                }
            }
        }

        UnloadTexture(debug);
        debug = LoadTextureFromImage(image);
        UnloadImage(image);
        return debug;
    }

} // namespace fin
