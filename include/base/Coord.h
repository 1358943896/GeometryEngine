#pragma once

#include <iosfwd>
#include <algorithm>

namespace geo
{
    class Coord
    {
    public:
        double x, y;

        Coord();

        Coord(double x, double y);

        Coord operator+(const Coord &rhs) const;

        Coord operator-(const Coord &rhs) const;

        Coord operator*(double scalar) const;

        Coord operator/(double scalar) const;

        Coord &operator+=(const Coord &rhs);

        Coord &operator-=(const Coord &rhs);

        Coord &operator*=(double scalar);

        Coord operator-() const;

        bool operator==(const Coord &rhs) const;

        bool equal(const Coord &rhs, double eps) const;

        bool operator!=(const Coord &rhs) const;

        double distanceTo(const Coord &other) const;

        friend std::ostream &operator<<(std::ostream &os, const Coord &c);
    };
} // namespace geo
