#pragma once

#include "earcut.hpp"
#include "math_utils.hpp"
#include "triangle.h"

#include <algorithm>
#include <queue>
#include <span>
#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>
/*
namespace mapbox
{
    namespace util
    {
        template <>
        struct nth<0, fin::Vec2f>
        {
            static auto get(const fin::Vec2f& p)
            {
                return p.x;
            }
        };
        template <>
        struct nth<1, fin::Vec2f>
        {
            static auto get(const fin::Vec2f& p)
            {
                return p.y;
            }
        };
    } // namespace util
} // namespace mapbox

namespace std
{
    template <>
    struct hash<fin::Vec2f>
    {
        size_t operator()(const fin::Vec2f& v) const
        {
            size_t h1 = std::hash<float>()(v.x);
            size_t h2 = std::hash<float>()(v.y);
            return h1 ^ (h2 << 1);
        }
    };

    template <>
    struct hash<std::pair<fin::Vec2f, fin::Vec2f>>
    {
        size_t operator()(const std::pair<fin::Vec2f, fin::Vec2f>& p) const
        {
            size_t h1 = hash<fin::Vec2f>()(p.first);
            size_t h2 = hash<fin::Vec2f>()(p.second);
            return h1 ^ (h2 << 1);
        }
    };
} // namespace std

namespace fin
{
    struct Vec2Hash
    {
        std::size_t operator()(const Vec2f& v) const
        {
            auto h1 = std::hash<float>{}(std::round(v.x * 1000.0f)); // rounded to avoid float precision garbage
            auto h2 = std::hash<float>{}(std::round(v.y * 1000.0f));
            return h1 ^ (h2 << 1);
        }
    };

    struct Vec2EdgeHash
    {
        std::size_t operator()(const std::pair<Vec2f, Vec2f>& e) const
        {
            Vec2Hash h;
            return h(e.first) ^ (h(e.second) << 1);
        }
    };

    class Pathfinder
    {
        struct Neighbor
        {
            int   triangleIndex;
            Vec2f portalA, portalB; // shared edge
        };

        struct Triangle
        {
            Vec2f verts[3];
            bool  walkable = true; // new

            bool Contains(const Vec2f& p) const
            {
                // Barycentric method
                auto sign = [](const Vec2f& p1, const Vec2f& p2, const Vec2f& p3)
                { return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y); };

                bool b1 = sign(p, verts[0], verts[1]) < 0.0f;
                bool b2 = sign(p, verts[1], verts[2]) < 0.0f;
                bool b3 = sign(p, verts[2], verts[0]) < 0.0f;

                return (b1 == b2) && (b2 == b3);
            }

            Vec2f center() const
            {
                return {(verts[0].x + verts[1].x + verts[2].x) / 3.0f, (verts[0].y + verts[1].y + verts[2].y) / 3.0f};
            }
        };

    public:
        void Clear(const Rectf& rc);
        void AddObstacle(std::span<Vec2f> polygon);
        void Generate();

        bool FindPath(Vec2f from, Vec2f to, std::vector<Vec2f>& path);

        int  FindTriangleContaining(const Vec2f& p);

        std::span<const Triangle> GetDebugTriangels() const;

    protected:
        void  BuildAdjacency();
        float TriArea2(const Vec2f& a, const Vec2f& b, const Vec2f& c) const;
        bool  Funnel(const Vec2f& start, const Vec2f& end, const std::vector<int>& triPath, std::vector<Vec2f>& result);
        float SignedArea(const std::vector<Vec2f>& ring);
        void  EnsureWinding(std::vector<Vec2f>& ring, bool desiredCCW);
        void  SmoothPath(std::vector<Vec2f>& path, const std::vector<Triangle>& triangles);
        bool IsLineWalkable(const Vec2f& a, const Vec2f& b, const std::vector<Triangle>& triangles);

        std::vector<std::vector<Neighbor>> m_neighbors;
        std::vector<Vec2f>                 m_allVerts;
        std::vector<std::vector<Vec2f>>    m_combined;
        std::vector<Triangle>              m_triangles;
    };


    inline void Pathfinder::Clear(const Rectf& rc)
    {
        m_combined.clear();
        m_triangles.clear();
        m_neighbors.clear();

        auto& ve = m_combined.emplace_back();
        ve.push_back(rc.top_left());     // 0,0
        ve.push_back(rc.top_right());    // 1,0
        ve.push_back(rc.bottom_right()); // 1,1
        ve.push_back(rc.bottom_left());  // 0,1

        EnsureWinding(ve, true);
    }

    inline void Pathfinder::AddObstacle(std::span<Vec2f> polygon)
    {
        if (polygon.empty())
            return;

        auto& ring = m_combined.emplace_back(polygon.begin(), polygon.end());
        EnsureWinding(ring, false);
    }

    bool PointInPolygon(const Vec2f& p, const std::vector<Vec2f>& poly)
    {
        if (poly.size() < 3)
            return false;

        bool        inside  = false;
        const float epsilon = 0.0001f;

        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
        {
            const Vec2f& vi = poly[i];
            const Vec2f& vj = poly[j];

            // Check if point is exactly on a vertex
            if ((p - vi).length_squared() < epsilon)
                return true;

            // Check if point is on edge
            if (std::abs((vi - p).cross(vj - p)) < epsilon && (p - vi).dot(p - vj) <= 0)
            {
                return true;
            }

            // Standard crossing test
            if (((vi.y > p.y) != (vj.y > p.y)) && (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y + epsilon) + vi.x))
            {
                inside = !inside;
            }
        }
        return inside;
    }

    inline void Pathfinder::Generate()
    {
        m_triangles.clear();
        m_allVerts.clear();

        std::vector<Vec2f>               allPoints;
        std::vector<std::pair<int, int>> segments;
        std::vector<Vec2f>               holeCenters;

        int vertexOffset = 0;

        for (size_t ringIndex = 0; ringIndex < m_combined.size(); ++ringIndex)
        {
            const auto& ring = m_combined[ringIndex];
            if (ring.empty())
                continue;

            // Add vertices and segments
            for (size_t i = 0; i < ring.size(); ++i)
            {
                allPoints.push_back(ring[i]);
                int next = (i + 1) % ring.size();
                segments.emplace_back(vertexOffset + int(i), vertexOffset + next);
            }

            // Holes: compute centroid for each ring (Triangle expects a point inside the hole)
            if (ringIndex > 0)
            {
                Vec2f center{0, 0};
                for (const auto& p : ring)
                    center += p;
                center /= float(ring.size());
                holeCenters.push_back(center);
            }

            vertexOffset += static_cast<int>(ring.size());
        }

        // Set up Triangle input
        triangulateio in{}, out{};

        in.numberofpoints = static_cast<int>(allPoints.size());
        in.pointlist      = (float*)allPoints.data();

        in.numberofsegments = static_cast<int>(segments.size());
        in.segmentlist      = (int*)segments.data();

        // Set up holes (center point inside each)
        in.numberofholes = static_cast<int>(holeCenters.size());
        in.holelist      = (float*)holeCenters.data();

        in.numberofpointattributes = 0;

        // Run triangulation
        char options[] = "zpq0";
        triangulate(options, &in, &out, nullptr);

        // Store triangles
        for (int i = 0; i < out.numberoftriangles; ++i)
        {
            int i0 = out.trianglelist[i * 3 + 0] * 2;
            int i1 = out.trianglelist[i * 3 + 1] * 2;
            int i2 = out.trianglelist[i * 3 + 2] * 2;

            Triangle tri{Vec2f{out.pointlist[i0], out.pointlist[i0 + 1]},
                         Vec2f{out.pointlist[i1], out.pointlist[i1 + 1]},
                         Vec2f{out.pointlist[i2], out.pointlist[i2 + 1]}};
            tri.walkable = true;

            for (size_t n = 1; n < m_combined.size(); ++n)
            {
                auto& hole = m_combined[n];
                if (PointInPolygon(tri.center(), hole))
                {
                    tri.walkable = false;
                    break;
                }
            }

            m_triangles.push_back(tri);
        }

        m_allVerts = allPoints;

        // Cleanup
        free(out.pointlist);
        free(out.trianglelist);

        BuildAdjacency();
    }

    inline bool Pathfinder::FindPath(Vec2f from, Vec2f to, std::vector<Vec2f>& path)
    {
        struct Node
        {
            int   tri;
            float cost, total;
            int   parent;

            bool operator>(const Node& o) const
            {
                return total > o.total;
            }
        };

        // Early out if start/end are in same triangle
        int startTri = FindTriangleContaining(from);
        int endTri   = FindTriangleContaining(to);
        if (startTri == endTri && startTri >= 0 && endTri >= 0)
        {
            path = {from, to};
            return true;
        }

        if (startTri < 0)
            return true;
        if (endTri < 0)
            return true;

        std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
        std::unordered_map<int, float>                               gScore;
        std::unordered_map<int, int>                                 cameFrom;

        open.push({startTri, 0, m_triangles[startTri].center().distance_squared(to), -1});
        gScore[startTri]   = 0;
        cameFrom[startTri] = -1;

        while (!open.empty())
        {
            Node current = open.top();
            open.pop();

            if (current.tri == endTri)
            {
                cameFrom[endTri] = current.parent;
                break;
            }

            for (const auto& neighbor : m_neighbors[current.tri])
            {
                float moveCost = m_triangles[current.tri].center().distance_squared(m_triangles[neighbor.triangleIndex].center());

                float tentative = gScore[current.tri] + moveCost;
                if (!gScore.contains(neighbor.triangleIndex) || tentative < gScore[neighbor.triangleIndex])
                {
                    gScore[neighbor.triangleIndex] = tentative;
                    float heuristic                = m_triangles[neighbor.triangleIndex].center().distance_squared(to);
                    open.push({neighbor.triangleIndex, tentative, tentative + heuristic, current.tri});
                    cameFrom[neighbor.triangleIndex] = current.tri;
                }
            }
        }

        // Reconstruct triangle path
        std::vector<int> triPath;
        for (int at = endTri; at != -1; at = cameFrom[at])
            triPath.push_back(at);
        std::reverse(triPath.begin(), triPath.end());

        if (Funnel(from, to, triPath, path))
        {
            SmoothPath(path, m_triangles); // Add this line
            return true;
        }
        return false;
    }

    inline bool Pathfinder::IsLineWalkable(const Vec2f& a, const Vec2f& b, const std::vector<Triangle>& triangles)
    {
        // Raycast through all triangles along the line segment
        Vec2f direction = (b - a).normalized();
        float maxDist   = a.distance(b);

        // Check start and end triangles first
        int currentTri = FindTriangleContaining(a);
        if (currentTri < 0 || !triangles[currentTri].walkable)
            return false;

        float       traveled = 0.0f;
        const float step     = 0.1f; // Resolution of the check

        while (traveled < maxDist)
        {
            Vec2f point = a + direction * traveled;
            int   tri   = FindTriangleContaining(point);
            if (tri < 0 || !triangles[tri].walkable)
                return false;

            // Accelerate check when in same triangle
            if (tri == currentTri)
            {
                traveled += step * 2;
                continue;
            }

            currentTri = tri;
            traveled += step;
        }
        return true;
    }

    inline void Pathfinder::SmoothPath(std::vector<Vec2f>& path, const std::vector<Triangle>& triangles)
    {
        if (path.size() < 3)
            return;

        std::vector<Vec2f> newPath;
        newPath.push_back(path[0]);

        size_t lastValid = 0;
        for (size_t i = 1; i < path.size(); ++i)
        {
            if (!IsLineWalkable(newPath.back(), path[i], triangles))
            {
                newPath.push_back(path[i - 1]);
                lastValid = i - 1;
            }
        }

        newPath.push_back(path.back());
        path = std::move(newPath);
    }

    inline int Pathfinder::FindTriangleContaining(const Vec2f& p)
    {
        for (int i = 0; i < m_triangles.size(); ++i)
        {
            if (m_triangles[i].Contains(p))
                return i;
        }
        return -1;
    }

    inline std::span<const Pathfinder::Triangle> Pathfinder::GetDebugTriangels() const
    {
        return m_triangles;
    }

    inline bool Pathfinder::Funnel(const Vec2f& start, const Vec2f& end, const std::vector<int>& triPath, std::vector<Vec2f>& out)
    {
        if (triPath.empty())
            return false;
        if (triPath.size() == 1)
        {
            out = {start, end};
            return true;
        }

        std::vector<Vec2f> portalsLeft, portalsRight;

        // Add start portal
        portalsLeft.push_back(start);
        portalsRight.push_back(start);

        // Collect portals between triangles
        for (size_t i = 0; i < triPath.size() - 1; ++i)
        {
            int currentTri = triPath[i];
            int nextTri    = triPath[i + 1];

            bool found = false;
            for (const auto& neighbor : m_neighbors[currentTri])
            {
                if (neighbor.triangleIndex == nextTri)
                {
                    // Get the edge from current triangle's perspective
                    const Triangle& tri = m_triangles[currentTri];
                    Vec2f           edgeStart, edgeEnd;

                    // Find which edge this is in current triangle
                    for (int j = 0; j < 3; ++j)
                    {
                        Vec2f a = tri.verts[j];
                        Vec2f b = tri.verts[(j + 1) % 3];

                        if ((a == neighbor.portalA && b == neighbor.portalB) ||
                            (a == neighbor.portalB && b == neighbor.portalA))
                        {
                            // Use triangle's natural winding order
                            edgeStart = a;
                            edgeEnd   = b;
                            break;
                        }
                    }

                    // Determine correct portal orientation
                    Vec2f toNext  = m_triangles[nextTri].center() - tri.center();
                    Vec2f edgeDir = edgeEnd - edgeStart;

                    if (edgeDir.cross(toNext) < 0)
                    {
                        portalsLeft.push_back(edgeStart);
                        portalsRight.push_back(edgeEnd);
                    }
                    else
                    {
                        portalsLeft.push_back(edgeEnd);
                        portalsRight.push_back(edgeStart);
                    }

                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }

        // Add end portal
        portalsLeft.push_back(end);
        portalsRight.push_back(end);

        // Funnel algorithm core
        out.clear();
        out.push_back(start);

        Vec2f  apex       = start;
        Vec2f  leftBound  = portalsLeft[1];
        Vec2f  rightBound = portalsRight[1];
        size_t leftIndex = 1, rightIndex = 1;

        for (size_t i = 2; i < portalsLeft.size(); ++i)
        {
            // Handle zero-length segments
            if ((portalsLeft[i] - apex).length_squared() < 0.0001f || (portalsRight[i] - apex).length_squared() < 0.0001f)
            {
                continue;
            }

            // Update right vertex
            if (TriArea2(apex, rightBound, portalsRight[i]) <= 0.0f)
            {
                if (apex == rightBound || TriArea2(apex, leftBound, portalsRight[i]) <= 0.0f)
                {
                    rightBound = portalsRight[i];
                    rightIndex = i;
                }
                else
                {
                    out.push_back(leftBound);
                    apex       = leftBound;
                    i          = leftIndex + 1;
                    leftBound  = portalsLeft[i];
                    rightBound = portalsRight[i];
                    leftIndex  = i;
                    rightIndex = i;
                    continue;
                }
            }

            // Update left vertex
            if (TriArea2(apex, leftBound, portalsLeft[i]) >= 0.0f)
            {
                if (apex == leftBound || TriArea2(apex, rightBound, portalsLeft[i]) >= 0.0f)
                {
                    leftBound = portalsLeft[i];
                    leftIndex = i;
                }
                else
                {
                    out.push_back(rightBound);
                    apex       = rightBound;
                    i          = rightIndex + 1;
                    leftBound  = portalsLeft[i];
                    rightBound = portalsRight[i];
                    leftIndex  = i;
                    rightIndex = i;
                    continue;
                }
            }
        }

        out.push_back(end);
        return true;
    }

    inline float Pathfinder::SignedArea(const std::vector<Vec2f>& ring)
    {
        float area = 0.0f;
        for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
        {
            area += (ring[j].x - ring[i].x) * (ring[j].y + ring[i].y);
        }
        return area;
    }

    inline void Pathfinder::EnsureWinding(std::vector<Vec2f>& ring, bool desiredCCW)
    {
        if (ring.size() < 3)
            return; // Not a valid polygon

        float area = SignedArea(ring);

        // CCW: area should be negative
        bool isCCW = area < 0.0f;

        if (isCCW != desiredCCW)
        {
            std::reverse(ring.begin(), ring.end());
        }
    }

    inline float Pathfinder::TriArea2(const Vec2f& a, const Vec2f& b, const Vec2f& c) const
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    inline void Pathfinder::BuildAdjacency()
    {
        using EdgeKey = std::pair<Vec2f, Vec2f>;

        auto canonical = [](const Vec2f& a, const Vec2f& b) -> EdgeKey
        { return (a.x < b.x || (a.x == b.x && a.y <= b.y)) ? std::make_pair(a, b) : std::make_pair(b, a); };

        struct EdgeInfo
        {
            int   triangleIndex;
            int   edgeIndex; // which edge in triangle (0,1,2)
            Vec2f a, b;
        };

        std::unordered_map<EdgeKey, std::vector<EdgeInfo>> edgeMap;

        // Build edge map
        for (int i = 0; i < static_cast<int>(m_triangles.size()); ++i)
        {
            const Triangle& tri = m_triangles[i];
            for (int e = 0; e < 3; ++e)
            {
                Vec2f a = tri.verts[e];
                Vec2f b = tri.verts[(e + 1) % 3];
                edgeMap[canonical(a, b)].push_back({i, e, a, b});
            }
        }

        m_neighbors.clear();
        m_neighbors.resize(m_triangles.size());

        for (const auto& [key, infos] : edgeMap)
        {
            // Connect all adjacent triangles
            for (size_t i = 0; i < infos.size(); ++i)
            {
                for (size_t j = i + 1; j < infos.size(); ++j)
                {
                    const EdgeInfo& e0 = infos[i];
                    const EdgeInfo& e1 = infos[j];

                    if (m_triangles[e0.triangleIndex].walkable && m_triangles[e1.triangleIndex].walkable)
                    {
                        m_neighbors[e0.triangleIndex].push_back({e1.triangleIndex, e0.a, e0.b});
                        m_neighbors[e1.triangleIndex].push_back({e0.triangleIndex, e1.a, e1.b});
                    }
                }
            }
        }
    }

} // namespace fin
*/



namespace fin
{

    class Pathfinder
    {
    public:
        struct Triangle
        {
            std::array<Vec2f, 3> vertices;
            Vec2f                center;
            std::array<int, 3>   neighbors{-1, -1, -1};
            bool                 walkable = true;

            Vec2f calculate_center() const
            {
                return {(vertices[0].x + vertices[1].x + vertices[2].x) / 3.0f,
                        (vertices[0].y + vertices[1].y + vertices[2].y) / 3.0f};
            }
        };

        struct PathResult
        {
            std::vector<Vec2f> path;
            std::vector<int>   trianglePath;
            std::vector<std::pair<Vec2f, Vec2f>> portals;
            bool               success = false;
        };

    private:
        std::vector<std::vector<Vec2f>>                       m_combined;
        std::vector<Vec2f>                                    m_allVerts;
        std::vector<Triangle>                                 m_triangles;
        std::unordered_map<uint64_t, std::pair<Vec2f, Vec2f>> m_portals;

        // Helper functions
        float cross(const Vec2f& a, const Vec2f& b) const
        {
            return a.x * b.y - a.y * b.x;
        }

        float triangle_area(const Vec2f& a, const Vec2f& b, const Vec2f& c) const
        {
            return std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
        }

        uint64_t make_edge_key(int t1, int t2) const
        {
            return (static_cast<uint64_t>(std::min(t1, t2)) << 32) | std::max(t1, t2);
        }

        float SignedArea(const std::vector<Vec2f>& ring)
        {
            float area = 0.0f;
            for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
            {
                area += (ring[j].x - ring[i].x) * (ring[j].y + ring[i].y);
            }
            return area;
        }

        void EnsureWinding(std::vector<Vec2f>& ring, bool desiredCCW)
        {
            if (ring.size() < 3)
                return; // Not a valid polygon

            float area = SignedArea(ring);

            // CCW: area should be negative
            bool isCCW = area < 0.0f;

            if (isCCW != desiredCCW)
            {
                std::reverse(ring.begin(), ring.end());
            }
        }

        bool PointInPolygon(const Vec2f& p, const std::vector<Vec2f>& poly)
        {
            if (poly.size() < 3)
                return false;

            bool        inside  = false;
            const float epsilon = 0.0001f;

            for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
            {
                const Vec2f& vi = poly[i];
                const Vec2f& vj = poly[j];

                // Check if point is exactly on a vertex
                if ((p - vi).length_squared() < epsilon)
                    return true;

                // Check if point is on edge
                if (std::abs((vi - p).cross(vj - p)) < epsilon && (p - vi).dot(p - vj) <= 0)
                {
                    return true;
                }

                // Standard crossing test
                if (((vi.y > p.y) != (vj.y > p.y)) && (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y + epsilon) + vi.x))
                {
                    inside = !inside;
                }
            }
            return inside;
        }

    public:
        void Clear(const Rectf& rc)
        {
            m_combined.clear();
            m_triangles.clear();

            auto& ve = m_combined.emplace_back();
            ve.push_back(rc.top_left());     // 0,0
            ve.push_back(rc.top_right());    // 1,0
            ve.push_back(rc.bottom_right()); // 1,1
            ve.push_back(rc.bottom_left());  // 0,1

            EnsureWinding(ve, true);
        }

        void AddObstacle(std::span<Vec2f> polygon)
        {
            if (polygon.empty())
                return;

            auto& ring = m_combined.emplace_back(polygon.begin(), polygon.end());
            EnsureWinding(ring, false);
        }

        void Generate()
        {
            m_triangles.clear();
            m_allVerts.clear();

            std::vector<Vec2f>               allPoints;
            std::vector<std::pair<int, int>> segments;
            std::vector<Vec2f>               holeCenters;

            int vertexOffset = 0;

            for (size_t ringIndex = 0; ringIndex < m_combined.size(); ++ringIndex)
            {
                const auto& ring = m_combined[ringIndex];
                if (ring.empty())
                    continue;

                // Add vertices and segments
                for (size_t i = 0; i < ring.size(); ++i)
                {
                    allPoints.push_back(ring[i]);
                    int next = (i + 1) % ring.size();
                    segments.emplace_back(vertexOffset + int(i), vertexOffset + next);
                }

                // Holes: compute centroid for each ring (Triangle expects a point inside the hole)
                if (ringIndex > 0)
                {
                    Vec2f center{0, 0};
                    for (const auto& p : ring)
                        center += p;
                    center /= float(ring.size());
                    holeCenters.push_back(center);
                }

                vertexOffset += static_cast<int>(ring.size());
            }

            // Set up Triangle input
            triangulateio in{}, out{};

            in.numberofpoints = static_cast<int>(allPoints.size());
            in.pointlist      = (float*)allPoints.data();

            in.numberofsegments = static_cast<int>(segments.size());
            in.segmentlist      = (int*)segments.data();

            // Set up holes (center point inside each)
            in.numberofholes = static_cast<int>(holeCenters.size());
            in.holelist      = (float*)holeCenters.data();

            in.numberofpointattributes = 0;

            // Run triangulation
            char options[] = "nzpq0";
            triangulate(options, &in, &out, nullptr);

            // Store triangles and build neighbor relationships
            m_triangles.resize(out.numberoftriangles); // Pre-allocate for direct access

            for (int i = 0; i < out.numberoftriangles; ++i)
            {
                // Get vertex indices for this triangle
                int v0 = out.trianglelist[i * 3 + 0];
                int v1 = out.trianglelist[i * 3 + 1];
                int v2 = out.trianglelist[i * 3 + 2];

                // Create triangle with vertices
                Triangle tri;
                tri.vertices[0] = Vec2f(out.pointlist[v0 * 2], out.pointlist[v0 * 2 + 1]);
                tri.vertices[1] = Vec2f(out.pointlist[v1 * 2], out.pointlist[v1 * 2 + 1]);
                tri.vertices[2] = Vec2f(out.pointlist[v2 * 2], out.pointlist[v2 * 2 + 1]);
                tri.center   = tri.calculate_center();

                // Set neighbors from triangulate output
                tri.neighbors[0] = out.neighborlist[i * 3 + 0]; // Edge v0-v1
                tri.neighbors[1] = out.neighborlist[i * 3 + 1]; // Edge v1-v2
                tri.neighbors[2] = out.neighborlist[i * 3 + 2]; // Edge v2-v0

                // Hole detection
                tri.walkable = true;
                for (size_t n = 1; n < m_combined.size(); ++n)
                {
                    if (PointInPolygon(tri.center, m_combined[n]))
                    {
                        tri.walkable = false;
                        break;
                    }
                }

                m_triangles[i] = tri;
            }

            // Post-process neighbors to handle boundary edges
            for (auto& tri : m_triangles)
            {
                for (int& neighbor : tri.neighbors)
                {
                    // Convert Triangle's -1 boundary markers to our -1 convention
                    if (neighbor == -1)
                        continue; // Already correct

                    // Validate neighbor index
                    if (neighbor < 0 || neighbor >= static_cast<int>(m_triangles.size()))
                    {
                        neighbor = -1; // Invalid index, treat as boundary
                    }
                }
            }

            m_allVerts = allPoints;

            // Cleanup
            free(out.pointlist);
            free(out.trianglelist);
            free(out.neighborlist);

            build();
        }

        std::span<const Pathfinder::Triangle> GetDebugTriangels() const
        {
            return m_triangles;
        }

        PathResult FindPath(const Vec2f& start, const Vec2f& end) const
        {
            PathResult result;

            // Phase 1: Find containing triangles
            int start_tri = find_containing_triangle(start);
            int end_tri   = find_containing_triangle(end);

            if (start_tri == -1 || end_tri == -1 || !m_triangles[start_tri].walkable || !m_triangles[end_tri].walkable)
            {
                return result;
            }

            // Phase 2: A* through triangle network
            result.trianglePath = a_star_search(start_tri, end_tri);
            if (result.trianglePath.empty())
                return result;

            // Phase 3: Funnel algorithm
            result.success = funnel_algorithm(start, end, result);

            // Phase 4: Path optimization
            if (result.success)
            {
            //    optimize_path(result.path);
            }

            return result;
        }

        // Debug visualization
        void get_debug_data(std::vector<Vec2f>& edges, std::vector<Vec2f>& portals) const
        {
            edges.clear();
            portals.clear();

            // Triangle edges
            for (const auto& tri : m_triangles)
            {
                if (!tri.walkable)
                    continue;

                for (int i = 0; i < 3; ++i)
                {
                    edges.push_back(tri.vertices[i]);
                    edges.push_back(tri.vertices[(i + 1) % 3]);
                }
            }

            // Portals
            for (const auto& [key, portal] : m_portals)
            {
                portals.push_back(portal.first);
                portals.push_back(portal.second);
            }
        }

    private:
        void build()
        {
            // Precompute portals and neighbors
            for (size_t i = 0; i < m_triangles.size(); ++i)
            {
                auto& tri  = m_triangles[i];
                tri.center = tri.calculate_center();

                for (int edge = 0; edge < 3; ++edge)
                {
                    int neighbor = tri.neighbors[edge];
                    if (neighbor != -1 && neighbor > static_cast<int>(i))
                    {
                        auto& ntri = m_triangles[neighbor];

                        Vec2f edge_p1;
                        Vec2f edge_p2;
                        int shared_count = 0;
                        for (int j = 0; j < 3; ++j)
                        {
                            for (int k = 0; k < 3; ++k)
                            {
                                if (tri.vertices[j] == ntri.vertices[k])
                                {
                                    if (shared_count == 0)
                                        edge_p1 = tri.vertices[j];
                                    else
                                        edge_p2 = tri.vertices[j];
                                    shared_count++;
                                    break;
                                }
                            }
                            if (shared_count == 2)
                                break;
                        }

                        m_portals[make_edge_key(i, neighbor)] = {edge_p1, edge_p2};
                    }
                }
            }
        }

        // Core algorithms
        std::vector<int> a_star_search(int start, int end) const
        {
            struct Node
            {
                int   index;
                float cost;
                float heuristic;

                bool operator>(const Node& other) const
                {
                    return (cost + heuristic) > (other.cost + other.heuristic);
                }
            };

            std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
            std::unordered_map<int, float>                               cost_map;
            std::unordered_map<int, int>                                 came_from;

            open.push({start, 0.0f, distance(m_triangles[start].center, m_triangles[end].center)});
            cost_map[start] = 0.0f;
            came_from[start] = -1;

            while (!open.empty())
            {
                Node current = open.top();
                open.pop();

                if (current.index == end)
                    break;

                for (int neighbor : m_triangles[current.index].neighbors)
                {
                    if (neighbor == -1 || !m_triangles[neighbor].walkable)
                        continue;

                    float new_cost = cost_map[current.index] +
                                     distance(m_triangles[current.index].center, m_triangles[neighbor].center);

                    if (!cost_map.count(neighbor) || new_cost < cost_map[neighbor])
                    {
                        cost_map[neighbor] = new_cost;
                        float heuristic    = distance(m_triangles[neighbor].center, m_triangles[end].center);
                        open.push({neighbor, new_cost, heuristic});
                        came_from[neighbor] = current.index;
                    }
                }
            }

            // SAFE PATH RECONSTRUCTION
            std::vector<int> path;
            if (!came_from.count(end))
            {
                return path; // No path exists
            }

            int                     current = end;
            std::unordered_set<int> path_nodes;

            while (current != -1 && !path_nodes.count(current))
            {
                path.push_back(current);
                path_nodes.insert(current);
                current = came_from[current];

                // Prevent infinite loops from bad data
                if (path.size() > m_triangles.size())
                {
                    path.clear();
                    break;
                }
            }

            if (current != -1)
            {
                // Invalid path - clear result
                path.clear();
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

       bool funnel_algorithm(const Vec2f&            start,
                              const Vec2f&     end,
                              PathResult& result) const
        {
            constexpr float EPSILONs = 1e-5f;
            result.path.clear();

            if (result.trianglePath.size() < 2)
            {
                result.path = {start, end};
                return true;
            }

            // Portal initialization (correct as is)
            result.portals.clear();

            const auto first_edge = make_edge_key(result.trianglePath[0], result.trianglePath[1]);
            if (auto it = m_portals.find(first_edge); it != m_portals.end())
            {
                result.portals.push_back(it->second);
            }

            for (size_t i = 2; i < result.trianglePath.size(); ++i)
            {
                const auto edge = make_edge_key(result.trianglePath[i - 1], result.trianglePath[i]);
                if (auto it = m_portals.find(edge); it != m_portals.end())
                {
                    result.portals.push_back(it->second);
                }
            }

            if (result.portals.empty())
            {
                result.path = {start, end};
                return true;
            }
      
            // Initialize funnel
            Vec2f apex  = start;
            Vec2f left  = result.portals[0].first;
            Vec2f right = result.portals[0].second;
            result.path.push_back(apex);

            const size_t MAX_ITERATIONS = result.portals.size() * 4;
            size_t       iterations     = 0;

            for (size_t i = 1; i < result.portals.size() && iterations++ < MAX_ITERATIONS;)
            {
                const Vec2f new_left  = result.portals[i].first;
                const Vec2f new_right = result.portals[i].second;

                const float cross_right = triangle_area(apex, right, new_right);
                const float cross_left  = triangle_area(apex, left, new_left);

                // Handle right boundary
                if (cross_right < -EPSILONs)
                {
                    if (triangle_area(apex, left, new_right) < -EPSILONs)
                    {
                        right = new_right;
                        i++;
                        continue;
                    }
                    result.path.push_back(left);
                    apex = left;
                    // Update to current portal's edges and increment i
                    left  = new_left;
                    right = new_right;
                    i++;
                    continue;
                }

                // Handle left boundary
                if (cross_left > EPSILONs)
                {
                    if (triangle_area(apex, right, new_left) > EPSILONs)
                    {
                        left = new_left;
                        i++;
                        continue;
                    }
                    result.path.push_back(right);
                    apex = right;
                    // Update to current portal's edges and increment i
                    left  = new_left;
                    right = new_right;
                    i++;
                    continue;
                }

                // Move to next portal
                i++;
            }

            // Add end point if necessary
            if (result.path.empty() || result.path.back().distance_squared(end) > EPSILONs * EPSILONs)
            {
                result.path.push_back(end);
            }

            return !result.path.empty();
        }

        void optimize_path(std::vector<Vec2f>& path) const
        {
            if (path.size() < 3)
                return;

            size_t write_index = 1;
            for (size_t i = 2; i < path.size(); ++i)
            {
                if (!has_line_of_sight(path[write_index - 1], path[i]))
                {
                    path[write_index++] = path[i - 1];
                }
            }

            path[write_index++] = path.back();
            path.erase(path.begin() + write_index, path.end());
        }

        // Utility functions
        int find_containing_triangle(const Vec2f& p) const
        {
            for (size_t i = 0; i < m_triangles.size(); ++i)
            {
                if (m_triangles[i].walkable && point_in_triangle(p, m_triangles[i]))
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        bool point_in_triangle(const Vec2f& p, const Triangle& tri) const
        {
            Vec2f v0 = tri.vertices[2] - tri.vertices[0];
            Vec2f v1 = tri.vertices[1] - tri.vertices[0];
            Vec2f v2 = p - tri.vertices[0];

            float dot00 = v0.dot(v0);
            float dot01 = v0.dot(v1);
            float dot02 = v0.dot(v2);
            float dot11 = v1.dot(v1);
            float dot12 = v1.dot(v2);

            float inv_denom = 1.0f / (dot00 * dot11 - dot01 * dot01);
            float u         = (dot11 * dot02 - dot01 * dot12) * inv_denom;
            float v         = (dot00 * dot12 - dot01 * dot02) * inv_denom;

            return (u >= 0) && (v >= 0) && (u + v < 1.0f);
        }

        bool has_line_of_sight(const Vec2f& a, const Vec2f& b) const
        {
            const float step     = 0.5f; // Navmesh resolution dependent
            Vec2f       dir      = (b - a).normalized();
            float       distance = a.distance(b);

            for (float t = 0; t < distance; t += step)
            {
                Vec2f p = a + dir * t;
                if (find_containing_triangle(p) == -1)
                {
                    return false;
                }
            }
            return true;
        }

        static float distance(const Vec2f& a, const Vec2f& b)
        {
            Vec2f diff = b - a;
            return std::sqrt(diff.x * diff.x + diff.y * diff.y);
        }
    };

} // namespace fin
