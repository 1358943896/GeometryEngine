#pragma once
#include <numeric>

namespace geo
{
    struct BoundingBox
    {
        double left, bottom, right, top;

        double width() const { return right - left; }
        double height() const { return top - bottom; }
        double area() const { return width() * height(); }

        bool contains(double x, double y) const
        {
            return x >= left && x <= right && y >= bottom && y <= top;
        }

        bool intersects(const BoundingBox &o) const
        {
            return left <= o.right && right >= o.left &&
                   bottom <= o.top && top >= o.bottom;
        }

        static BoundingBox unite(const BoundingBox &a, const BoundingBox &b)
        {
            return {
                std::min(a.left, b.left), std::min(a.bottom, b.bottom),
                std::max(a.right, b.right), std::max(a.top, b.top)
            };
        }
    };
} // namespace geo
