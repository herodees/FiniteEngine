#pragma once

#include <vector>
#include <map>
#include <utility>
	

#include "point.h"
#include "polygon.h"
#include "segment.h"
#include <span>
#include <unordered_set>


namespace NavMesh {

	class PathFinder
	{
	public:
		// Call one time when map changes.
		// Resets the map and creates the graph.
		//
		// |polygons_to_add| - convex polygons on the map.
		// |inflate_by| - how far away paths must go from any polygon
		void AddPolygons(const std::vector<Polygon>& polygons_to_add, int inflate_by);
		
		// Call any time after AddPolygons().
		// It removes previously added external points and adds
		// |points| to the graph.
	    void AddExternalPoints(std::span<Point> points_);

        std::span<const Point> GetExternalPoints() const;
		// Get shortest path between two points.
		// points must be first added via AddExternalPoints().
		std::span<const Point> GetPath(const Point& start_coord, const Point& dest_coord);

		// For debugging. Returns all the edges in the graph.
		std::vector<Segment> GetEdgesForDebug(bool external) const;
	private:
		int GetVertex(const Point& c);
		void AddEdge(int be, int en);
		bool CanAddSegment(const Segment& s, const std::vector<std::pair<int, int>>& tangents);

		std::vector<Polygon> polygons_;
		std::vector<Point> ext_points_;
        std::unordered_set<int> external_vertex_ids_;

		std::vector<int> free_vertices_;
		std::map<Point, int> vertex_ids_;
		std::vector<Point> v_;
        std::vector<Point> path_;

		std::vector<std::vector<bool>> polygon_point_is_inside_;
		std::vector<std::vector<std::pair<int, double>>> edges_;
	};


}
