#include "geometry/GeoPoint.h"
#include <ostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace geo
{
    GeoPoint::GeoPoint(const Coord &coord) : coords_({coord})
    {
    }

    GeoPoint::GeoPoint(double x, double y) : coords_({{x, y}})
    {
    }

    GeoPoint::GeoPoint(PartList coords) : coords_(std::move(coords))
    {
    }

    std::size_t GeoPoint::partCount() const
    {
        // 返回部件（独立点）数量
        return coords_.size();
    }

    const Coord &GeoPoint::part(std::size_t i) const
    {
        // 按索引访问指定部件坐标
        return coords_.at(i);
    }

    void GeoPoint::addPart(const Coord &coord)
    {
        // 追加一个新的点部件
        coords_.push_back(coord);
    }

    const Coord &GeoPoint::coord() const
    {
        // 单部件兼容接口，返回第一个部件坐标
        return coords_.at(0);
    }

    double GeoPoint::x() const { return coords_.at(0).x; }
    double GeoPoint::y() const { return coords_.at(0).y; }

    GeoPoint::PartList::const_iterator GeoPoint::begin() const { return coords_.begin(); }
    GeoPoint::PartList::const_iterator GeoPoint::end() const { return coords_.end(); }
    GeoPoint::PartList::iterator GeoPoint::begin() { return coords_.begin(); }
    GeoPoint::PartList::iterator GeoPoint::end() { return coords_.end(); }

    BoundingBox GeoPoint::boundingBox() const
    {
        if (coords_.empty()) return {0, 0, 0, 0};
        // 遍历所有部件，计算整体包围盒
        double minX = coords_[0].x, minY = coords_[0].y;
        double maxX = minX, maxY = minY;
        for (const auto &c : coords_) {
            minX = std::min(minX, c.x);
            minY = std::min(minY, c.y);
            maxX = std::max(maxX, c.x);
            maxY = std::max(maxY, c.y);
        }
        return {minX, minY, maxX, maxY};
    }

    std::size_t GeoPoint::vertexCount() const
    {
        // 每个部件贡献一个顶点
        return coords_.size();
    }

    const Coord &GeoPoint::vertex(std::size_t i) const
    {
        // 按扁平索引访问第 i 个部件坐标
        return coords_.at(i);
    }

    double GeoPoint::area() const { return 0.0; }
    double GeoPoint::perimeter() const { return 0.0; }

    Geometry::GeometryType GeoPoint::type() const { return Geometry::GeometryType::Point; }

    std::unique_ptr<Geometry> GeoPoint::clone() const
    {
        return std::make_unique<GeoPoint>(*this);
    }

    bool GeoPoint::operator==(const GeoPoint &rhs) const { return coords_ == rhs.coords_; }
    bool GeoPoint::operator!=(const GeoPoint &rhs) const { return !(*this == rhs); }

    std::ostream &operator<<(std::ostream &os, const GeoPoint &p)
    {
        // 单部件直接输出坐标，多部件输出所有部件
        if (p.coords_.size() == 1) {
            return os << "GeoPoint" << p.coords_[0];
        }
        os << "MultiPoint[";
        for (std::size_t i = 0; i < p.coords_.size(); ++i) {
            if (i) os << ", ";
            os << p.coords_[i];
        }
        return os << "]";
    }
} // namespace geo
