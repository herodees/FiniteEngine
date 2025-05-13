#pragma once

#include "earcut.hpp"
#include "math_utils.hpp"
#include <span>
#include <string>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include "triangle.h"


namespace mapbox {
    namespace util {
        template <>
        struct nth<0, fin::Vec2f> { static auto get(const fin::Vec2f& p) { return p.x; } };
        template <>
        struct nth<1, fin::Vec2f> { static auto get(const fin::Vec2f& p) { return p.y; } };
    }
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
        struct Triangle
        {
            Vec2f a, b, c;

            bool Contains(const Vec2f& p) const
            {
                // Barycentric method
                auto sign = [](const Vec2f& p1, const Vec2f& p2, const Vec2f& p3)
                { return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y); };

                bool b1 = sign(p, a, b) < 0.0f;
                bool b2 = sign(p, b, c) < 0.0f;
                bool b3 = sign(p, c, a) < 0.0f;

                return (b1 == b2) && (b2 == b3);
            }

            std::vector<Vec2f> Vertices() const
            {
                return {a, b, c};
            }

            float AspectRatio() const
            {
                float ab = (b - a).length();
                float bc = (c - b).length();
                float ca = (a - c).length();

                float longest   = std::max({ab, bc, ca});
                float perimeter = ab + bc + ca;
                float s         = perimeter / 2.0f;

                // Heron's formula for area
                float area = std::sqrt(std::max(0.0f, s * (s - ab) * (s - bc) * (s - ca)));

                // Inradius = Area / s
                float inradius = (s > 0.0f) ? (area / s) : 1e-6f;

                // Aspect ratio = Longest side / (2 * Inradius)
                float aspect = (2 * inradius > 0.0f) ? (longest / (2.0f * inradius)) : 9999.0f;
                return aspect;
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
        void                                      BuildAdjacency();
        float TriArea2(const Vec2f& a, const Vec2f& b, const Vec2f& c) const;
        bool  AStar(int start, int goal, const Vec2f& from, const Vec2f& to, std::vector<int>& path);
        Vec2f CenterOf(const Triangle& t) const;
        bool  Funnel(const Vec2f& start, const Vec2f& end, const std::vector<int>& triPath, std::vector<Vec2f>& result);
        void BuildVertexGraph();

        inline float SignedArea(const std::vector<Vec2f>& ring)
        {
            float area = 0.0f;
            for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
            {
                area += (ring[j].x - ring[i].x) * (ring[j].y + ring[i].y);
            }
            return area;
        }

        inline void EnsureWinding(std::vector<Vec2f>& ring, bool desiredCCW)
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

        struct Neighbor
        {
            int   triangleIndex;
            Vec2f edgeA, edgeB;
        };

        std::unordered_map<int, std::vector<Neighbor>>          m_neighbors;
        std::vector<Vec2f>                        m_allVerts;
        std::vector<std::vector<Vec2f>>           m_combined;
        std::vector<uint32_t>                     m_indices;
        std::vector<Triangle>                     m_triangles;
        std::unordered_map<Vec2f, std::vector<Vec2f>, Vec2Hash> m_vertexGraph;
    };



    inline void Pathfinder::Clear(const Rectf& rc)
    {
        m_combined.clear();
        m_indices.clear();
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

        if (m_combined.size())
            return;

        auto& ring = m_combined.emplace_back(polygon.begin(), polygon.end());
        EnsureWinding(ring, false);
    }

    inline void Pathfinder::Generate()
    {
        /*
        using N = uint32_t;

        // Flatten all rings
        m_allVerts.clear();

        for (const auto& ring : m_combined)
        {
            m_allVerts.insert(m_allVerts.end(), ring.begin(), ring.end());
        }

        // Triangulate
        m_indices = mapbox::earcut<N>(m_combined);

        // Build triangles from flattened vertex list
        m_triangles.clear();
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            Triangle tri{m_allVerts[m_indices[i]], m_allVerts[m_indices[i + 1]], m_allVerts[m_indices[i + 2]]};
            m_triangles.push_back(tri);
        }

        BuildAdjacency();
        BuildVertexGraph();
        */

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
        in.pointlist      = (float*)malloc(sizeof(float) * 2 * in.numberofpoints);
        for (int i = 0; i < in.numberofpoints; ++i)
        {
            in.pointlist[i * 2 + 0] = allPoints[i].x;
            in.pointlist[i * 2 + 1] = allPoints[i].y;
        }

        in.numberofsegments = static_cast<int>(segments.size());
        in.segmentlist      = (int*)malloc(sizeof(int) * 2 * in.numberofsegments);
        for (int i = 0; i < in.numberofsegments; ++i)
        {
            in.segmentlist[i * 2 + 0] = segments[i].first;
            in.segmentlist[i * 2 + 1] = segments[i].second;
        }

        // Set up holes (center point inside each)
        in.numberofholes = static_cast<int>(holeCenters.size());
        if (in.numberofholes > 0)
        {
            in.holelist = (float*)malloc(sizeof(float) * 2 * in.numberofholes);
            for (int i = 0; i < in.numberofholes; ++i)
            {
                in.holelist[i * 2 + 0] = holeCenters[i].x;
                in.holelist[i * 2 + 1] = holeCenters[i].y;
            }
        }

        in.numberofpointattributes = 0;

        // Run triangulation
        char options[] = "pq0";
        triangulate(options, &in, &out, nullptr);

        // Store triangles
        for (int i = 0; i < out.numberoftriangles; ++i)
        {
            int i0 = out.trianglelist[i * 3 + 0];
            int i1 = out.trianglelist[i * 3 + 1];
            int i2 = out.trianglelist[i * 3 + 2];

            Triangle tri{Vec2f{out.pointlist[i0 * 2], out.pointlist[i0 * 2 + 1]},
                         Vec2f{out.pointlist[i1 * 2], out.pointlist[i1 * 2 + 1]},
                         Vec2f{out.pointlist[i2 * 2], out.pointlist[i2 * 2 + 1]}};

            m_triangles.push_back(tri);
        }

        m_allVerts = allPoints;

        // Cleanup
        free(in.pointlist);
        free(in.segmentlist);
        free(in.holelist);
        free(out.pointlist);
        free(out.trianglelist);

        BuildAdjacency();
        BuildVertexGraph();
    }

    inline bool Pathfinder::FindPath(Vec2f from, Vec2f to, std::vector<Vec2f>& path)
    {
        struct TriNode
        {
            int   index;
            float cost;
            float total;
            int   parent;

            bool operator>(const TriNode& o) const
            {
                return total > o.total;
            }
        };

        int startTri = FindTriangleContaining(from);
        int endTri   = FindTriangleContaining(to);
        if (startTri < 0 || endTri < 0)
            return false;

        std::priority_queue<TriNode, std::vector<TriNode>, std::greater<>> open;
        std::unordered_map<int, float>                                     gScore;
        std::unordered_map<int, int>                                       cameFrom;

        open.push({startTri, 0.0f, CenterOf(m_triangles[startTri]).distance(to), -1});
        gScore[startTri] = 0.0f;
        cameFrom[startTri] = -1;

        while (!open.empty())
        {
            TriNode current = open.top();
            open.pop();
            if (current.index == endTri)
            {
                cameFrom[endTri] = current.parent;
                break;
            }

            for (const auto& neighbor : m_neighbors[current.index])
            {
                float newCost = gScore[current.index] +
                                CenterOf(m_triangles[current.index]).distance(CenterOf(m_triangles[neighbor.triangleIndex]));
                if (!gScore.contains(neighbor.triangleIndex) || newCost < gScore[neighbor.triangleIndex])
                {
                    gScore[neighbor.triangleIndex] = newCost;
                    float heuristic                = CenterOf(m_triangles[neighbor.triangleIndex]).distance(to);
                    open.push({neighbor.triangleIndex, newCost, newCost + heuristic, current.index});
                    cameFrom[neighbor.triangleIndex] = current.index;
                }
            }
        }

        std::vector<int> triPath;
        for (int at = endTri; at != -1; at = cameFrom[at])
        {
            triPath.push_back(at);
        }
        std::reverse(triPath.begin(), triPath.end());

        // Funnel smoothing
        return Funnel(from, to, triPath, path);
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

    inline Vec2f Pathfinder::CenterOf(const Triangle& t) const
    {
        return {(t.a.x + t.b.x + t.c.x) / 3.0f, (t.a.y + t.b.y + t.c.y) / 3.0f};
    }

    inline bool Pathfinder::Funnel(const Vec2f& start, const Vec2f& end, const std::vector<int>& triPath, std::vector<Vec2f>& result)
    {
        struct Portal
        {
            Vec2f left, right;
        };

        std::vector<Portal> portals;
        portals.push_back({start, start});

        // Build portals between triangles using shared edges
        for (size_t i = 0; i + 1 < triPath.size(); ++i)
        {
            int a = triPath[i];
            int b = triPath[i + 1];

            bool found = false;
            for (const auto& neighbor : m_neighbors[a])
            {
                if (neighbor.triangleIndex == b)
                {
                    // Determine proper left/right winding
                    Vec2f dir     = (end - start).normalized();
                    Vec2f edgeDir = (neighbor.edgeB - neighbor.edgeA).normalized();
                    float cross   = dir.cross(edgeDir);

                    if (cross >= 0.0f)
                        portals.push_back({neighbor.edgeA, neighbor.edgeB});
                    else
                        portals.push_back({neighbor.edgeB, neighbor.edgeA});

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                // fallback — shouldn't happen if adjacency is correct
                continue;
            }
        }

        portals.push_back({end, end});

        // Funnel algorithm
        result.clear();
        Vec2f  apex      = start;
        Vec2f  left      = start;
        Vec2f  right     = start;
        size_t apexIndex = 0, leftIndex = 0, rightIndex = 0;

        result.push_back(apex);

        for (size_t i = 1; i < portals.size(); ++i)
        {
            const Vec2f& newLeft  = portals[i].left;
            const Vec2f& newRight = portals[i].right;

            // Tighten right
            if (TriArea2(apex, right, newRight) <= 0.0f)
            {
                if (apex == right || TriArea2(apex, left, newRight) > 0.0f)
                {
                    right      = newRight;
                    rightIndex = i;
                }
                else
                {
                    result.push_back(left);
                    apex       = left;
                    apexIndex  = leftIndex;
                    left       = apex;
                    right      = apex;
                    leftIndex  = apexIndex;
                    rightIndex = apexIndex;
                    i          = apexIndex;
                    continue;
                }
            }

            // Tighten left
            if (TriArea2(apex, left, newLeft) >= 0.0f)
            {
                if (apex == left || TriArea2(apex, right, newLeft) < 0.0f)
                {
                    left      = newLeft;
                    leftIndex = i;
                }
                else
                {
                    result.push_back(right);
                    apex       = right;
                    apexIndex  = rightIndex;
                    left       = apex;
                    right      = apex;
                    leftIndex  = apexIndex;
                    rightIndex = apexIndex;
                    i          = apexIndex;
                    continue;
                }
            }
        }

        result.push_back(end);
        return true;
    }



    inline void Pathfinder::BuildVertexGraph()
    {
        m_vertexGraph.clear();
        for (const auto& tri : m_triangles)
        {
            auto verts = tri.Vertices();
            for (int i = 0; i < 3; ++i)
            {
                Vec2f a = verts[i];
                Vec2f b = verts[(i + 1) % 3];
                m_vertexGraph[a].push_back(b);
                m_vertexGraph[b].push_back(a);
            }
        }
    }

    inline float Pathfinder::TriArea2(const Vec2f& a, const Vec2f& b, const Vec2f& c) const
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    inline void Pathfinder::BuildAdjacency()
    {
        m_neighbors.clear();
        std::unordered_map<std::pair<Vec2f, Vec2f>, std::vector<int>, Vec2EdgeHash> edgeMap;

        auto makeKey = [](Vec2f a, Vec2f b) { return std::make_pair(std::min(a, b), std::max(a, b)); };

        for (int i = 0; i < m_triangles.size(); ++i)
        {
            auto verts = m_triangles[i].Vertices();
            for (int j = 0; j < 3; ++j)
            {
                Vec2f a = verts[j];
                Vec2f b = verts[(j + 1) % 3];
                edgeMap[makeKey(a, b)].push_back(i);
            }
        }

        for (const auto& [edge, tris] : edgeMap)
        {
            if (tris.size() == 2)
            {
                int a = tris[0], b = tris[1];
                m_neighbors[a].push_back({b, edge.first, edge.second});
                m_neighbors[b].push_back({a, edge.first, edge.second});
            }
        }
    }
}
