#pragma once

#include "include.hpp"

namespace fin
{
	template<typename T, typename GetRect, typename Equal = std::equal_to<T>, typename Float = float>
	class quadtree
	{
		static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>, bool>,
			"Equal must be a callable of signature bool(const T&, const T&)");
		static_assert(std::is_arithmetic_v<Float>);

	public:
		quadtree(const Rectangle& box, const GetRect& getrect, const Equal& equal = Equal()) :
			mBox(box), mRoot(std::make_unique<Node>()), mGetRect(getrect), mEqual(equal)
		{

		}

		void clear(const Rectangle& box) {
			mBox = box;
			mRoot = std::make_unique<Node>();
		}

		void add(const T& value)
		{
			add(mRoot.get(), 0, mBox, value);
		}

		void remove(const T& value)
		{
			remove(mRoot.get(), mBox, value);
		}

		std::vector<T> query(const Rectangle& box) const
		{
			auto values = std::vector<T>();
			query(mRoot.get(), mBox, box, values);
			return values;
		}

        void query(const Rectangle& box, std::vector<T>& out) const
        {
            out.clear();
            query(mRoot.get(), mBox, box, out);
        }

		std::vector<std::pair<T, T>> findAllIntersections() const
		{
			auto intersections = std::vector<std::pair<T, T>>();
			findAllIntersections(mRoot.get(), intersections);
			return intersections;
		}

		Rectangle getBox() const
		{
			return mBox;
		}

	private:
		static constexpr auto Threshold = std::size_t(16);
		static constexpr auto MaxDepth = std::size_t(8);

		float getRight(const Rectangle& rc) const { return rc.x + rc.width; }
		float getBottom(const Rectangle& rc) const { return rc.y + rc.height; }
		Vec2f getTopLeft(const Rectangle& rc) const { return { rc.x, rc.y}; }
		Vec2f getCenter(const Rectangle& rc) const { return { rc.x + rc.width / 2, rc.y + rc.height / 2 }; }
		Vec2f get_size(const Rectangle& rc) const { return { rc.width, rc.height }; }
		bool contains(const Rectangle& rc, const Rectangle& box) const { return rc.x <= box.x && getRight(box) <= getRight(rc) && rc.y <= box.y && getBottom(box) <= getBottom(rc); }
		bool intersects(const Rectangle& rc, const Rectangle& box) const { return !(rc.x >= getRight(box) || getRight(rc) <= box.x ||rc.y >= getBottom(box) || getBottom(rc) <= box.y); }

		struct Node
		{
			std::array<std::unique_ptr<Node>, 4> children;
			std::vector<T> values;
		};

		Rectangle mBox;
		std::unique_ptr<Node> mRoot;
		Equal mEqual;
		GetRect mGetRect;

		bool isLeaf(const Node* node) const
		{
			return !static_cast<bool>(node->children[0]);
		}

		Rectangle computeBox(const Rectangle& box, int i) const
		{
			auto origin = getTopLeft(box);
			auto childSize = get_size(box);
			childSize.x /= 2;
			childSize.y /= 2;

			switch (i)
			{
				// North West
			case 0:
				return Rectangle(origin.x, origin.y, childSize.x, childSize.y);
				// Norst East
			case 1:
				return Rectangle(origin.x + childSize.x, origin.y, childSize.x, childSize.y);
				// South West
			case 2:
				return Rectangle(origin.x, origin.y + childSize.y, childSize.x, childSize.y);
				// South East
			case 3:
				return Rectangle(origin.x + childSize.x, origin.y + childSize.y, childSize.x, childSize.y);
			default:
				assert(false && "Invalid child index");
				return Rectangle();
			}
		}

		int getQuadrant(const Rectangle& nodeBox, const Rectangle& valueBox) const
		{
			auto center = getCenter(nodeBox);
			// West
			if (getRight(valueBox) < center.x)
			{
				// North West
				if (getBottom(valueBox) < center.y)
					return 0;
				// South West
				else if (valueBox.y >= center.y)
					return 2;
				// Not contained in any quadrant
				else
					return -1;
			}
			// East
			else if (valueBox.x >= center.x)
			{
				// North East
				if (getBottom(valueBox) < center.y)
					return 1;
				// South East
				else if (valueBox.y >= center.y)
					return 3;
				// Not contained in any quadrant
				else
					return -1;
			}
			// Not contained in any quadrant
			else
				return -1;
		}

		void add(Node* node, std::size_t depth, const Rectangle& box, const T& value)
		{
			assert(node != nullptr);
			assert(contains(box, mGetRect(value)));
			if (isLeaf(node))
			{
				// Insert the value in this node if possible
				if (depth >= MaxDepth || node->values.size() < Threshold)
					node->values.push_back(value);
				// Otherwise, we split and we try again
				else
				{
					split(node, box);
					add(node, depth, box, value);
				}
			}
			else
			{
				auto i = getQuadrant(box, mGetRect(value));
				// Add the value in a child if the value is entirely contained in it
				if (i != -1)
					add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computeBox(box, i), value);
				// Otherwise, we add the value in the current node
				else
					node->values.push_back(value);
			}
		}

		void split(Node* node, const Rectangle& box)
		{
			assert(node != nullptr);
			assert(isLeaf(node) && "Only leaves can be split");
			// Create children
			for (auto& child : node->children)
				child = std::make_unique<Node>();
			// Assign values to children
			auto newValues = std::vector<T>(); // New values for this node
			for (const auto& value : node->values)
			{
				auto i = getQuadrant(box, mGetRect(value));
				if (i != -1)
					node->children[static_cast<std::size_t>(i)]->values.push_back(value);
				else
					newValues.push_back(value);
			}
			node->values = std::move(newValues);
		}

		bool remove(Node* node, const Rectangle& box, const T& value)
		{
			assert(node != nullptr);
			assert(contains(box, mGetRect(value)));
			if (isLeaf(node))
			{
				// Remove the value from node
				removeValue(node, value);
				return true;
			}
			else
			{
				// Remove the value in a child if the value is entirely contained in it
				auto i = getQuadrant(box, mGetRect(value));
				if (i != -1)
				{
					if (remove(node->children[static_cast<std::size_t>(i)].get(), computeBox(box, i), value))
						return tryMerge(node);
				}
				// Otherwise, we remove the value from the current node
				else
					removeValue(node, value);
				return false;
			}
		}

		void removeValue(Node* node, const T& value)
		{
			// Find the value in node->values
			auto it = std::find_if(std::begin(node->values), std::end(node->values),
				[this, &value](const auto& rhs) { return mEqual(value, rhs); });
			assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");
			// Swap with the last element and pop back
			*it = std::move(node->values.back());
			node->values.pop_back();
		}

		bool tryMerge(Node* node)
		{
			assert(node != nullptr);
			assert(!isLeaf(node) && "Only interior nodes can be merged");
			auto nbValues = node->values.size();
			for (const auto& child : node->children)
			{
				if (!isLeaf(child.get()))
					return false;
				nbValues += child->values.size();
			}
			if (nbValues <= Threshold)
			{
				node->values.reserve(nbValues);
				// Merge the values of all the children
				for (const auto& child : node->children)
				{
					for (const auto& value : child->values)
						node->values.push_back(value);
				}
				// Remove the children
				for (auto& child : node->children)
					child.reset();
				return true;
			}
			else
				return false;
		}

		void query(Node* node, const Rectangle& box, const Rectangle& queryBox, std::vector<T>& values) const
		{
			assert(node != nullptr);
			assert(intersects(queryBox, box));
			for (const auto& value : node->values)
			{
				if (intersects(queryBox, mGetRect(value)))
					values.push_back(value);
			}
			if (!isLeaf(node))
			{
				for (auto i = std::size_t(0); i < node->children.size(); ++i)
				{
					auto childBox = computeBox(box, static_cast<int>(i));
					if (intersects(queryBox, childBox))
						query(node->children[i].get(), childBox, queryBox, values);
				}
			}
		}

		void findAllIntersections(Node* node, std::vector<std::pair<T, T>>& intersections) const
		{
			// Find intersections between values stored in this node
			// Make sure to not report the same intersection twice
			for (auto i = std::size_t(0); i < node->values.size(); ++i)
			{
				for (auto j = std::size_t(0); j < i; ++j)
				{
					if (intersects(mGetRect(node->values[i]), mGetRect(node->values[j])))
						intersections.emplace_back(node->values[i], node->values[j]);
				}
			}
			if (!isLeaf(node))
			{
				// Values in this node can intersect values in descendants
				for (const auto& child : node->children)
				{
					for (const auto& value : node->values)
						findIntersectionsInDescendants(child.get(), value, intersections);
				}
				// Find intersections in children
				for (const auto& child : node->children)
					findAllIntersections(child.get(), intersections);
			}
		}

		void findIntersectionsInDescendants(Node* node, const T& value, std::vector<std::pair<T, T>>& intersections) const
		{
			// Test against the values stored in this node
			for (const auto& other : node->values)
			{
				if (intersects(mGetRect(value), mGetRect(other)))
					intersections.emplace_back(value, other);
			}
			// Test against values stored into descendants of this node
			if (!isLeaf(node))
			{
				for (const auto& child : node->children)
					findIntersectionsInDescendants(child.get(), value, intersections);
			}
		}
	};

}
