#include "path_finder.h"

#include <queue>
#include <algorithm>


namespace NavMesh {

	void PathFinder::AddPolygons(const std::vector<Polygon>& polygons_to_add, int inflate_by = 0)
	{
		polygons_.clear();
		v_.clear();
		edges_.clear();
		vertex_ids_.clear();
		polygons_.reserve(polygons_to_add.size());
		for (auto const& p : polygons_to_add) {
			polygons_.emplace_back(p.Inflate(inflate_by));
			// Don't add polygons which are not really an obstacle.
			if (polygons_.back().Size() < 1) polygons_.pop_back();
		}

		// Calculate which polygon points are bad, i.e. inside some other polygon.
		polygon_point_is_inside_.resize(polygons_.size());
		for (size_t i = 0; i < polygons_.size(); ++i) {
			int n = polygons_[i].Size();
			polygon_point_is_inside_[i].resize(n);
			for (int k = 0; k < n; ++k) {
				polygon_point_is_inside_[i][k] = false;
				const Point& cur_point = polygons_[i].points_[k];
				for (int j = 0; j < polygons_.size(); ++j) {
					if (i != j && polygons_[j].IsInside(cur_point)) {
						polygon_point_is_inside_[i][k] = true;
						break;
					}
				}
			}
		}

		// Stores tangents from the currently considered point to all the polygons.
		std::vector<std::pair<int, int>> tangents(polygons_.size());

		// Consider all points and add edges from them.
		for (size_t i = 0; i < polygons_.size(); ++i) {
			const auto& cur_poly = polygons_[i];
			int n = cur_poly.Size();
			for (int k = 0; k < n; ++k) {
				if (polygon_point_is_inside_[i][k]) {
					// Current point is strictly inside something.
					// Can't add any edges from it.
					continue;
				}
				const Point& cur_point = cur_poly.points_[k];

				// Pre-calculate all tangents to all polygons to
				// speed-up intersection detection of possible edges.
				for (int j = 0; j < polygons_.size(); ++j) {
					tangents[j] = polygons_[j].GetTangentIds(cur_point);
				}

				// Add sides of polygon from this point. Only one is added here,
				// as the other one is added when the previous point is considered.
				Segment s = Segment(cur_point, cur_poly.points_[(k + 1) % n]);
				if (!polygon_point_is_inside_[i][(k + 1) % n] && CanAddSegment(s, tangents)) {
					AddEdge(GetVertex(s.b), GetVertex(s.e));
				}
				// Add tangents between polygons.
				for (size_t j = i + 1; j < polygons_.size(); ++j) {
					const auto& other_poly = polygons_[j];
					const auto& ids = tangents[j];
					// This point is equal to some vertex.
					// No sense adding edges to this polygon.
					if (ids.first == ids.second) continue;
					// Only consider the segment if it's also tangent to the current polygon.
					if (ids.first >= 0 && cur_poly.IsTangent(k, other_poly.points_[ids.first])) {
						s = Segment(cur_poly.points_[k], other_poly.points_[ids.first]);
						if (!polygon_point_is_inside_[j][ids.first] && CanAddSegment(s, tangents)) {
							AddEdge(GetVertex(s.b), GetVertex(s.e));
						}
					}
					if (ids.second >= 0 && cur_poly.IsTangent(k, other_poly.points_[ids.second])) {
						s = Segment(cur_poly.points_[k], other_poly.points_[ids.second]);
						if (!polygon_point_is_inside_[j][ids.second] && CanAddSegment(s, tangents)) {
							AddEdge(GetVertex(s.b), GetVertex(s.e));
						}
					}
				}
			}
		}
	}

    void PathFinder::AddExternalPoints(std::span<Point> points_)
    {
        // Remove old external points from the graph
        for (const auto& p : ext_points_) {
            auto it = vertex_ids_.find(p);
            if (it == vertex_ids_.end()) continue;
            int id = it->second;
            free_vertices_.push_back(id);
            for (const auto& e : edges_[id]) {
                int u = e.first;
                if (u == id) continue;
                auto& u_edges = edges_[u];
                for (size_t i = 0; i < u_edges.size(); ++i) {
                    if (u_edges[i].first == id) {
                        u_edges[i] = u_edges.back();
                        u_edges.pop_back();
                        break;
                    }
                }
            }
            edges_[id].clear();
            vertex_ids_.erase(it);
        }

        ext_points_.assign(points_.begin(), points_.end());
        std::vector<std::pair<int, int>> tangents(polygons_.size());

        // First pass: Handle points inside polygons
        for (auto& pt : ext_points_) {
            for (const auto& poly : polygons_) {
                if (poly.IsInside(pt)) {
                    // Fallback to closest point if no candidates
                    pt = poly.GetClosestPointOutside(pt);
                    break;
                }
            }
        }

        // Precompute all polygon vertices that are not inside other polygons
        std::vector<Point> valid_poly_vertices;
        for (size_t j = 0; j < polygons_.size(); ++j) {
            const auto& poly = polygons_[j];
            for (int k = 0; k < poly.Size(); ++k) {
                if (!polygon_point_is_inside_[j][k]) {
                    valid_poly_vertices.push_back(poly.points_[k]);
                }
            }
        }

        // Now add edges
        for (size_t i = 0; i < ext_points_.size(); ++i) {
            const Point& cur_point = ext_points_[i];

            // Precompute tangents for current point
            for (size_t j = 0; j < polygons_.size(); ++j) {
                tangents[j] = polygons_[j].GetTangentIds(cur_point);
            }

            // External-to-external edges
            for (size_t j = i + 1; j < ext_points_.size(); ++j) {
                if (CanAddSegment(Segment(cur_point, ext_points_[j]), tangents)) {
                    AddEdge(GetVertex(cur_point), GetVertex(ext_points_[j]));
                }
            }

            // External-to-polygon tangent points
            for (size_t j = 0; j < polygons_.size(); ++j) {
                const auto& ids = tangents[j];
                if (ids.first == -1 || ids.second == -1 || ids.first == ids.second)
                    continue;

                const Point& p1 = polygons_[j].points_[ids.first];
                const Point& p2 = polygons_[j].points_[ids.second];

                if (!polygon_point_is_inside_[j][ids.first] && CanAddSegment(Segment(cur_point, p1), tangents)) {
                    AddEdge(GetVertex(cur_point), GetVertex(p1));
                }

                if (!polygon_point_is_inside_[j][ids.second] && CanAddSegment(Segment(cur_point, p2), tangents)) {
                    AddEdge(GetVertex(cur_point), GetVertex(p2));
                }
            }

            // Additional visibility connections to all valid polygon vertices
            for (const auto& poly_pt : valid_poly_vertices) {
                // Skip if this is already connected via tangent
                bool already_connected = false;
                for (size_t j = 0; j < polygons_.size(); ++j) {
                    const auto& ids = tangents[j];
                    if ((ids.first >= 0 && poly_pt == polygons_[j].points_[ids.first]) ||
                        (ids.second >= 0 && poly_pt == polygons_[j].points_[ids.second])) {
                        already_connected = true;
                        break;
                    }
                }
            
                if (!already_connected && CanAddSegment(Segment(cur_point, poly_pt), tangents)) {
                    AddEdge(GetVertex(cur_point), GetVertex(poly_pt));
                }
            }
        }
    }

    std::span<const Point> PathFinder::GetExternalPoints() const
    {
        return ext_points_;
    }

	std::span<const Point> PathFinder::GetPath(const Point& start_coord, const Point& dest_coord)
    {
        path_.clear();

	    int start = GetVertex(start_coord);
	    int dest = GetVertex(dest_coord);

	    if (start == dest)
        {
            path_.push_back(start_coord);
            return path_;
        }

	    const size_t n = v_.size();
	    constexpr double INF = std::numeric_limits<double>::infinity();

	    std::vector<double> dist(n, INF);
	    std::vector<double> est(n, INF);
	    std::vector<int> prev(n, -1);
	    std::vector<bool> visited(n, false);

	    using QueueItem = std::pair<double, int>;
	    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;

	    dist[start] = 0.0;
	    est[start] = (v_[dest] - v_[start]).Len();
	    queue.emplace(est[start], start);

	    while (!queue.empty()) {
		    auto [cur_est, u] = queue.top();
		    queue.pop();
		    if (visited[u]) continue;
		    visited[u] = true;
		    if (u == dest) break;

		    for (const auto& [v, weight] : edges_[u]) {
			    double new_dist = dist[u] + weight;
			    if (new_dist < dist[v]) {
				    dist[v] = new_dist;
				    est[v] = new_dist + (v_[dest] - v_[v]).Len();
				    queue.emplace(est[v], v);
				    prev[v] = u;
			    }
		    }
	    }

	    if (prev[dest] == -1) return {}; // No path found

	    for (int u = dest; u != -1; u = prev[u])
		    path_.push_back(v_[u]);

	    std::reverse(path_.begin(), path_.end());
	    return path_;
    }


	std::vector<Segment> PathFinder::GetEdgesForDebug(bool external) const
	{
		std::vector<Segment> res;
        if (external)
        {
            for (auto& pt : ext_points_)
            {
                auto it = vertex_ids_.find(pt);
			    for (const auto& e : edges_[it->second])
                {
                    int i = edges_[it->second][0].first;
				    int j = e.first;
					res.push_back(Segment(v_[i], v_[j]));
			    }
            }
        }
        else
        {
            for (int i = 0; i < (int)edges_.size(); ++i)
            {
			    for (const auto& e : edges_[i])
                {
				    int j = e.first;
				    if (j > i) {
					    res.push_back(Segment(v_[i], v_[j]));
				    }
			    }
		    }
        }

		return res;
	}

	int PathFinder::GetVertex(const Point& c)
	{
		auto it = vertex_ids_.find(c);
		if (it != vertex_ids_.end()) {
			return it->second;
		}
		if (free_vertices_.empty()) {
			vertex_ids_[c] = (int)v_.size();
			v_.push_back(c);
			edges_.push_back({});
			return (int)v_.size() - 1;
		}
		else {
			int node = free_vertices_.back();
			free_vertices_.pop_back();
			v_[node] = c;
			vertex_ids_[c] = node;
			return node;
		}
	}

	void PathFinder::AddEdge(int be, int en)
	{
		double dst = (v_[be] - v_[en]).Len();
		edges_[be].push_back(std::make_pair(en, dst));
		edges_[en].push_back(std::make_pair(be, dst));
	}

	bool PathFinder::CanAddSegment(const Segment& s, const std::vector<std::pair<int, int>>& tangents)
	{
		for (size_t i = 0; i < polygons_.size(); ++i) {
			if (polygons_[i].Intersects(s, tangents[i])) return false;
		}
		return true;
	}

}
