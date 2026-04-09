#include "../../include/base/Coord.h"
#include <cmath>
#include <ostream>
#include <stdexcept>

namespace geo
{
    Coord::Coord() : x(0.0), y(0.0)
    {
    }

    Coord::Coord(double x, double y) : x(x), y(y)
    {
    }

    Coord Coord::operator+(const Coord &rhs) const { return {x + rhs.x, y + rhs.y}; }
    Coord Coord::operator-(const Coord &rhs) const { return {x - rhs.x, y - rhs.y}; }
    Coord Coord::operator*(double s) const { return {x * s, y * s}; }
    Coord Coord::operator/(double s) const { return {x / s, y / s}; }
    Coord Coord::operator-() const { return {-x, -y}; }

    Coord &Coord::operator+=(const Coord &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Coord &Coord::operator-=(const Coord &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Coord &Coord::operator*=(double s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    bool Coord::operator==(const Coord &rhs) const
    {
        constexpr double eps = 1e-9;
        return equal(rhs, eps);
    }

    bool Coord::equal(const Coord &rhs, const double eps) const
    {
        return std::abs(x - rhs.x) < eps && std::abs(y - rhs.y) < eps;
    }

    bool Coord::operator!=(const Coord &rhs) const { return !(*this == rhs); }

    double Coord::distanceTo(const Coord &o) const
    {
        double dx = x - o.x, dy = y - o.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    std::ostream &operator<<(std::ostream &os, const Coord &c)
    {
        return os << "(" << c.x << ", " << c.y << ")";
    }
} // namespace geo
