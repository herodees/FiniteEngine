#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fin
{
    template <typename T>
    struct Region;

    template <typename T>
    struct Vec2
    {
        union
        {
            struct { T x, y; };
            struct { T u, v; };
            struct { T width, height; };
            struct { T left, right; };
        };

        // Default constructor (zero-initialized)
        Vec2() : x{}, y{} {}

        // Parameterized constructor
        constexpr Vec2(T x, T y) : x(x), y(y) {}

        template <class OT>
        constexpr Vec2(const OT& ot) : x(ot.x), y(ot.y) {}

        // Unary negation
        Vec2 operator-() const { return Vec2(-x, -y); }

        // Arithmetic operators
        Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
        Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
        Vec2 operator*(T scalar) const { return Vec2(x * scalar, y * scalar); }
        Vec2 operator/(T scalar) const { return Vec2(x / scalar, y / scalar); }

        // Compound assignment operators
        Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
        Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
        Vec2& operator*=(T scalar) { x *= scalar; y *= scalar; return *this; }
        Vec2& operator/=(T scalar) { x /= scalar; y /= scalar; return *this; }

        // Comparison operators
        bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
        bool operator!=(const Vec2& other) const { return !(*this == other); }

        // Dot product
        auto dot(const Vec2& other) const -> std::common_type_t<T, decltype(x* other.x)> { return x * other.x + y * other.y; }
        // Cross product
        auto cross(const Vec2& other) const -> std::common_type_t<T, decltype(x* other.y)> { return x * other.y - y * other.x; }

        // Magnitude squared
        T length_squared() const { return x * x + y * y; }

        // Magnitude (length)
        T length() const { return std::sqrt(length_squared()); }

        // Normalize the vector
        Vec2 normalized() const { T len = length(); return (len > static_cast<T>(0)) ? (*this / len) : Vec2(); }

        Vec2 perp() const { return Vec2(-y, x); }

        Vec2 reflect(const Vec2& normal) const { return *this - 2 * dot(normal) * normal; }

        Vec2 lerp(const Vec2& other, T t) const { return (*this) * (1 - t) + other * t; }

        // Scalar multiplication (scalar * Vec2)
        friend Vec2 operator*(T scalar, const Vec2& vec) { return Vec2(scalar * vec.x, scalar * vec.y); }

        // Scalar addition (scalar + Vec2)
        friend Vec2 operator+(T scalar, const Vec2& vec) { return Vec2(scalar + vec.x, scalar + vec.y); }

        // Scalar subtraction (scalar - Vec2)
        friend Vec2 operator-(T scalar, const Vec2& vec) { return Vec2(scalar - vec.x, scalar - vec.y); }

        // Distance between two vectors
        T distance(const Vec2& other) const { return std::sqrt(distance_squared(other)); }

        // Distance squared between two vectors
        T distance_squared(const Vec2& other) const
        {
            T dx = x - other.x;
            T dy = y - other.y;
            return dx * dx + dy * dy;
        }

        T angle_degrees() const { return angle() * (180.0 / M_PI); }

        T angle() const { return std::atan2(y, x); }

        // Rotate the vector by an angle (in radians)
        Vec2 rotated(T radians) const
        {
            T cosA = std::cos(radians);
            T sinA = std::sin(radians);
            return Vec2(x * cosA - y * sinA, x * sinA + y * cosA);
        }

        // In-place rotation
        void rotate(T radians)
        {
            T cosA = std::cos(radians);
            T sinA = std::sin(radians);
            T newX = x * cosA - y * sinA;
            T newY = x * sinA + y * cosA;
            x = newX;
            y = newY;
        }

        // Rotate around a given center point (returns a new vector)
        Vec2 rotated_around(const Vec2& center, T radians) const
        {
            T cosA = std::cos(radians);
            T sinA = std::sin(radians);

            // Translate point to origin (center becomes (0,0))
            T dx = x - center.x;
            T dy = y - center.y;

            // Apply rotation
            T newX = dx * cosA - dy * sinA;
            T newY = dx * sinA + dy * cosA;

            // Translate back to original position
            return Vec2(newX + center.x, newY + center.y);
        }

        // In-place version (modifies this vector)
        void rotate_around(const Vec2& center, T radians) { *this = rotated_around(center, radians); }

        constexpr Vec2 iso2orth(const Vec2& tile) const
        {
            return Vec2(((x - y) * (tile.x / 2.f)), ((x + y) * (tile.y / 2.f)));
        }

        constexpr Vec2 orth2iso(const Vec2& tile) const
        {
            const float isox = ((x / (tile.x / 2.f) + y / (tile.y / 2.f)) / 2.f);
            const float isoy = ((y / (tile.y / 2.f) - (x / (tile.x / 2.f))) / 2.f);
            return Vec2(isox, isoy);
        }
    };

    using Vec2f = Vec2<float>;
    using Vec2i = Vec2<int32_t>;



    template <typename T>
    struct Rect
    {
        T x;
        T y;
        T width;
        T height;

        // Default constructor (zero-initialized)
        Rect() : x{}, y{}, width{}, height{} {}

        // Parameterized constructor
        Rect(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}

        // Get top-left corner
        Vec2<T> top_left() const { return Vec2<T>(x, y); }

        // Get top-right corner
        Vec2<T> top_right() const { return Vec2<T>(x + width, y); }

        // Get bottom-left corner
        Vec2<T> bottom_left() const { return Vec2<T>(x, y + height); }

        // Get bottom-right corner
        Vec2<T> bottom_right() const { return Vec2<T>(x + width, y + height); }

        // Get center
        Vec2<T> center() const { return Vec2<T>(x + width * static_cast<T>(0.5), y + height * static_cast<T>(0.5)); }
        // Get size
        Vec2<T> size() const { return Vec2<T>(width, height); }

        T x2() const { return x + width; }
        T y2() const { return y + height; }

        void set_center(T cx, T cy) {
            x = cx - width / 2;
            y = cy - height / 2;
        }
        void set_center(const Vec2<T>& c) { set_center(c.x, c.y); }

        Rect with_center(T cx, T cy) const { return Rect(cx - width / 2, cy - height / 2, width, height); }
        Rect with_center(const Vec2<T>& c) const { return with_center(c.x, c.y); }

        // Check if a point is inside the rectangle
        bool contains(T px, T py) const { return px >= x && px <= x + width && py >= y && py <= y + height; }

        bool contains(const Vec2<T>& point) const { return contains(point.x, point.y); }

        bool contains(const Rect& other) const
        {
            return other.x >= x && other.y >= y &&
                other.x + other.width <= x + width &&
                other.y + other.height <= y + height;
        }

        bool is_touching(const Rect& other) const
        {
            return (x + width == other.x || other.x + other.width == x ||
                y + height == other.y || other.y + other.height == y) &&
                !intersects(other);
        }

        // Check if this rectangle intersects another
        bool intersects(const Rect& other) const { return !(x + width < other.x || other.x + other.width < x || y + height < other.y || other.y + other.height < y); }

        // Expand (grow) the rectangle
        Rect expanded(T amount) const { return Rect(x - amount, y - amount, width + amount * 2, height + amount * 2); }

        // Move the rectangle
        Rect translated(T dx, T dy) const { return Rect(x + dx, y + dy, width, height); }

        Rect translated(const Vec2<T>& offset) const { return translated(offset.x, offset.y); }

        Rect overlap(const Rect& other) const
        {
            T overlap_x = std::max(x, other.x);
            T overlap_y = std::max(y, other.y);
            T overlap_width = std::min(x + width, other.x + other.width) - overlap_x;
            T overlap_height = std::min(y + height, other.y + other.height) - overlap_y;

            // If no valid overlap, return a zero-sized rect
            if (overlap_width <= 0 || overlap_height <= 0) {
                return Rect(0, 0, 0, 0);
            }

            return Rect(overlap_x, overlap_y, overlap_width, overlap_height);
        }

        // Get area
        T area() const { return width * height; }

        Region<T> region() const;

        // Equality check
        bool operator==(const Rect& other) const { return x == other.x && y == other.y && width == other.width && height == other.height; }
        bool operator!=(const Rect& other) const
        {
            return !(*this == other);
        }

        Rect operator+(const Vec2<T>& other) const
        {
            return Rect(x + other.x, y + other.y, width, height);
        }
        Rect operator-(const Vec2<T>& other) const
        {
            return Rect(x - other.x, y - other.y, width, height);
        }

        Rect& operator+=(const Vec2<T>& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }
        Rect& operator-=(const Vec2<T>& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }
    };

    using Rectf = Rect<float>;
    using Recti = Rect<int32_t>;
    using Rectangle = Rect<float>;



    template <typename T>
    struct Polygon
    {
        std::vector<Vec2<T>> vertices;
        mutable Rect<T> bbox;

        Polygon() = default;

        // Add a vertex to the polygon
        void add_vertex(const Vec2<T>& Vertex) { bbox.width = 0; vertices.push_back(Vertex); }

        // Check if a point is inside the polygon using the ray-casting algorithm
        bool contains(const Vec2<T>& point) const
        {
            bool inside = false;
            size_t n = vertices.size();
            for (size_t i = 0, j = n - 1; i < n; j = i++) {
                const Vec2<T>& v1 = vertices[i];
                const Vec2<T>& v2 = vertices[j];
                if (((v1.y > point.y) != (v2.y > point.y)) &&
                    (point.x < (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x)) {
                    inside = !inside;
                }
            }
            return inside;
        }

        // Rotate the polygon around a center point (returns a new rotated polygon)
        Polygon rotated_around(const Vec2<T>& center, T radians) const
        {
            Polygon rotated;
            for (const auto& Vertex : vertices) {
                rotated.add_vertex(Vertex.rotated_around(center, radians));
            }
            return rotated;
        }

        // In-place rotation of the polygon around a center point
        void rotate_around(const Vec2<T>& center, T radians)
        {
            bbox.width = 0;
            for (auto& Vertex : vertices) {
                Vertex = Vertex.rotated_around(center, radians);
            }
        }

        // Get the bounding box of the polygon
        Rect<T> bounding_box() const
        {
            if (bbox.width)
                return bbox;

            if (vertices.empty()) return Rect<T>();

            T min_x = vertices[0].x, max_x = vertices[0].x;
            T min_y = vertices[0].y, max_y = vertices[0].y;

            for (const auto& Vertex : vertices) {
                min_x = std::min(min_x, Vertex.x);
                max_x = std::max(max_x, Vertex.x);
                min_y = std::min(min_y, Vertex.y);
                max_y = std::max(max_y, Vertex.y);
            }

            bbox = Rect<T>(min_x, min_y, max_x - min_x, max_y - min_y);
            return bbox;
        }

        bool is_convex() const
        {
            if (vertices.size() < 3) return false; // A polygon must have at least 3 vertices

            bool is_positive = false; // Track the sign of the first non-zero cross product
            size_t n = vertices.size();

            // Iterate over each edge of the polygon
            for (size_t i = 0; i < n; i++) {
                // Current vertex
                const Vec2<T>& v0 = vertices[i];
                // Next vertex (wraps around)
                const Vec2<T>& v1 = vertices[(i + 1) % n];
                // Previous vertex (wraps around)
                const Vec2<T>& v2 = vertices[(i + 2) % n];

                // Compute cross product of the edge v0-v1 with the edge v1-v2
                T cross_product = (v1 - v0).cross(v2 - v1);

                if (cross_product != 0) {
                    // Check the sign of the cross product
                    if (is_positive == false) {
                        is_positive = cross_product > 0;
                    }
                    else if (is_positive != (cross_product > 0)) {
                        // If the sign of the cross product changes, the polygon is concave
                        return false;
                    }
                }
            }
            return true;
        }
    };

    using Polygonf = Polygon<float>;
    using Polygoni = Polygon<int32_t>;



    template <typename T>
    struct Region
    {
        T x1;
        T y1;
        T x2;
        T y2;

        Region() : x1{}, y1{}, x2{}, y2{} {}
        Region(T X1, T Y1, T X2, T Y2) : x1{ X1 }, y1{ Y1 }, x2{ X2 }, y2{ Y2 } {}
        Region(const Rect<T>& rc) : x1{ rc.x }, y1{ rc.y }, x2{ rc.x2() }, y2{ rc.y2() } {}

        void set_size(T w, T h) { x2 = x1 + w; y2 = y1 + h; }
        T width() const { return x2 - x1; }
        T height() const { return y2 - y1; }
        Vec2<T> size() const { return { x2 - x1, y2 - y1 }; }
        Vec2<T> center() const
        {
            return {(x1 + x2) / 2, (y1 + y2) / 2};
        }

        void move(T dx, T dy) { x1 += dx; x2 += dx; y1 += dy; y2 += dy; }
        void moveto(T x, T y)
        {
            x2 = width() + x;
            y2 = height() + y;
            x1 = x;
            y1 = y;
        }

        Rect<T> rect() const { return { x1, y1, x2 - x1, y2 - y1 }; }

        // Check if this rectangle intersects another
        bool intersects(const Region& other) const { return !(x2 < other.x1 || other.x2 < x1 || y2 < other.y1 || other.y2 < y1); }
        bool intersects(const Rect<T>& other) const { return !(x2 < other.x || other.x2() < x1 || y2 < other.y || other.y2() < y1); }
        
        bool contains(T px, T py) const { return px >= x1 && px <= x2 && py >= y1 && py <= y2; }
        bool contains(const Vec2<T>& point) const { return contains(point.x, point.y); }

        bool contains(const Rect<T>& other) const
        {
            return other.x >= x1 && other.y >= y1 && other.x + other.width <= x2 && other.y + other.height <= y2;
        }

        bool contains(const Region& other) const
        {
            return other.x1 >= x1 && other.y1 >= y1 && other.x2 <= x2 && other.y2 <= y2;
        }
    };

    template<typename T>
    inline Region<T> Rect<T>::region() const
    {
        return { x, y, x2(), y2() };
    }



    template <typename T>
    struct Line
    {
        Vec2<T> point1;
        Vec2<T> point2;

        bool is_point() const
        {
            return point1 == point2;
        }

        T get_y_at_x(T x) const
        {
            if (point1.x == point2.x) // Vertical line case
                return point1.y;

            T slope = (point2.y - point1.y) / (point2.x - point1.x);
            return point1.y + slope * (x - point1.x);
        }

        int compare(const Vec2<T>& point) const
        {
            if (point1.x == point2.x)
            {
                return point.x < point1.x ? -1 : (point.x > point1.x ? 1 : 0);
            }

            auto deltaX1 = point.x - point1.x;
            auto deltaY1 = point.y - point1.y;
            auto deltaX2 = point2.x - point1.x;
            auto deltaY2 = point2.y - point1.y;

            // Use cross-multiplication to avoid dividing
            auto cross1 = deltaY1 * deltaX2;
            auto cross2 = deltaY2 * deltaX1;

            if (cross1 == cross2)
            {
                return 0;  // Point is collinear with the line segment
            }

            return cross1 < cross2 ? -1 : 1;  // Determine if point is above or below the line
        }

        int compare(const Line<T>& ot) const
        {
            auto twoVSone = INT_MIN;
            auto oneVStwo = INT_MIN;

            auto comp1 = compare(ot.point1);
            if (comp1 == compare(ot.point2)) //Both points in line 1 are above or below line2
            {
                oneVStwo = comp1;
            }

            auto comp2 = ot.compare(point1);
            if (comp2 == ot.compare(point2)) //Both points in line 2 are above or below line1
            {
                twoVSone = -comp2;
            }

            if (oneVStwo != INT_MIN && twoVSone != INT_MIN)
            {
                if (oneVStwo == twoVSone) //the two comparisons agree about the ordering
                {
                    return oneVStwo;
                }
            }
            else if (oneVStwo != INT_MIN)
            {
                return oneVStwo;
            }
            else if (twoVSone != INT_MIN)
            {
                return twoVSone;
            }
  
            return compare_centers(ot);
        }


        int compare_centers(const Line<T>& ot) const
        {
            auto h1 = (point1.y + point2.y) / 2;
            auto h2 = (ot.point1.y + ot.point2.y) / 2;
            return h2 - h1;
        }

        Vec2f center() const
        {
            return { (point1.x + point2.x) / 2, (point1.y + point2.y) / 2 };
        }
    };
}
