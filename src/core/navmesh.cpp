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
    }

    void Navmesh::resize(float worldWidth, float worldHeight, int gridCellWidth, int gridCellHeight)
    {
        cell       = {gridCellWidth, gridCellHeight};
        size       = {worldWidth, worldHeight};
        cellsize.x = static_cast<int>(std::ceil(worldWidth / cell.x));
        cellsize.y = static_cast<int>(std::ceil(worldHeight / cell.y));
        terrain.resize(cellsize.x * cellsize.y);
        nodes.resize(cellsize.x * cellsize.y);
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

    void Navmesh::reconstructPath(Node& endNode, std::vector<Vec2i>& outPath) const
    {
        outPath.clear();
        Node* curr = &endNode;
        while (!(curr->x == curr->parentX && curr->y == curr->parentY))
        {
            outPath.push_back({curr->x, curr->y});
            curr = &nodes[curr->parentY * cellsize.x + curr->parentX];
        }
        outPath.push_back({curr->x, curr->y});
        std::reverse(outPath.begin(), outPath.end());
    }

    bool Navmesh::findPath(Vec2i start, Vec2i end, std::vector<Vec2i>& outPath) const
    {
        static constexpr int dx[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
        static constexpr int dy[8] = {0, 0, -1, 1, -1, 1, -1, 1};

        for (auto& n : nodes)
            n = Node();

        auto  idx       = [&](int x, int y) { return y * cellsize.x + x; };
        auto& startNode = nodes[idx(start.x, start.y)];
        startNode.x     = start.x;
        startNode.y     = start.y;
        startNode.gCost = 0;
        startNode.fCost = heuristic(start, end);
        startNode.open  = true;

        auto cmp = [&](int a, int b) { return nodes[a].fCost > nodes[b].fCost; };
        std::priority_queue<int, std::vector<int>, decltype(cmp)> open(cmp);
        open.push(idx(start.x, start.y));

        int   bestIdx  = -1;
        float bestDist = std::numeric_limits<float>::max();

        while (!open.empty())
        {
            int currentIdx = open.top();
            open.pop();
            Node& current  = nodes[currentIdx];
            current.closed = true;

            float distToGoal = heuristic({current.x, current.y}, end);
            if (distToGoal < bestDist)
            {
                bestDist = distToGoal;
                bestIdx  = currentIdx;
            }

            if (current.x == end.x && current.y == end.y)
            {
                reconstructPath(current, outPath);
                return true;
            }

            for (int d = 0; d < 8; ++d)
            {
                int nx = current.x + dx[d];
                int ny = current.y + dy[d];

                if (!isWalkable(nx, ny))
                    continue;

                int   ni       = idx(nx, ny);
                Node& neighbor = nodes[ni];
                if (neighbor.closed)
                    continue;

                float moveCost = current.gCost + cost(nx, ny) * (d < 4 ? 1.0f : 1.4142f);
                if (!neighbor.open || moveCost < neighbor.gCost)
                {
                    neighbor.x       = nx;
                    neighbor.y       = ny;
                    neighbor.gCost   = moveCost;
                    neighbor.fCost   = moveCost + heuristic({nx, ny}, end);
                    neighbor.parentX = current.x;
                    neighbor.parentY = current.y;
                    if (!neighbor.open)
                    {
                        neighbor.open = true;
                        open.push(ni);
                    }
                }
            }
        }

        if (bestIdx != -1)
        {
            reconstructPath(nodes[bestIdx], outPath);
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
        terrain.clear();
        terrain.resize(nodes.size());
    }

    const Texture2D& Navmesh::getDebugTexture() const
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

        debug.load_from_image(image);
        UnloadImage(image);
        return debug;
    }

} // namespace fin
