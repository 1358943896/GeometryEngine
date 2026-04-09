#include "geometry/GeoRegion.h"
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace geo
{
    GeoRegion::GeoRegion() = default;

    GeoRegion::GeoRegion(VertexList vertices) : parts_({std::move(vertices)})
    {
    }

    GeoRegion::GeoRegion(std::initializer_list<Coord> vertices) : parts_({VertexList(vertices)})
    {
    }

    GeoRegion::GeoRegion(PartList parts) : parts_(std::move(parts))
    {
    }

    std::size_t GeoRegion::partCount() const
    {
        return parts_.size();
    }

    const GeoRegion::VertexList &GeoRegion::part(std::size_t i) const
    {
        return parts_.at(i);
    }

    GeoRegion::VertexList &GeoRegion::part(std::size_t i)
    {
        return parts_.at(i);
    }

    void GeoRegion::addPart(VertexList vertices)
    {
        // 追加一个新的多边形部件
        parts_.push_back(std::move(vertices));
    }

    std::size_t GeoRegion::edgeCount() const
    {
        // 每个部件的边数等于其顶点数（闭合环）
        std::size_t total = 0;
        for (const auto &p : parts_) total += p.size();
        return total;
    }

    GeoRegion::PartList::const_iterator GeoRegion::begin() const { return parts_.begin(); }
    GeoRegion::PartList::const_iterator GeoRegion::end()   const { return parts_.end(); }
    GeoRegion::PartList::iterator       GeoRegion::begin()       { return parts_.begin(); }
    GeoRegion::PartList::iterator       GeoRegion::end()         { return parts_.end(); }

    BoundingBox GeoRegion::boundingBox() const
    {
        // 遍历所有部件所有顶点，计算整体包围盒
        double minX = std::numeric_limits<double>::max();
        double minY = minX, maxX = -minX, maxY = -minX;
        bool any = false;
        for (const auto &p : parts_) {
            for (const auto &v : p) {
                minX = std::min(minX, v.x); minY = std::min(minY, v.y);
                maxX = std::max(maxX, v.x); maxY = std::max(maxY, v.y);
                any = true;
            }
        }
        return any ? BoundingBox{minX, minY, maxX, maxY} : BoundingBox{0, 0, 0, 0};
    }

    std::size_t GeoRegion::vertexCount() const
    {
        // 累加所有部件的顶点数
        std::size_t total = 0;
        for (const auto &p : parts_) total += p.size();
        return total;
    }

    const Coord &GeoRegion::vertex(std::size_t i) const
    {
        // 按扁平索引定位顶点所在部件
        for (const auto &p : parts_) {
            if (i < p.size()) return p[i];
            i -= p.size();
        }
        throw std::out_of_range("GeoRegion::vertex index out of range");
    }

    double GeoRegion::area() const
    {
        // 累加所有部件的面积（Shoelace 公式）
        double total = 0.0;
        for (const auto &p : parts_) {
            std::size_t n = p.size();
            if (n < 3) continue;
            double sum = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                const Coord &a = p[i];
                const Coord &b = p[(i + 1) % n];
                sum += a.x * b.y - b.x * a.y;
            }
            total += std::abs(sum) * 0.5;
        }
        return total;
    }

    double GeoRegion::perimeter() const
    {
        // 累加所有部件的周长（含闭合边）
        double total = 0.0;
        for (const auto &p : parts_) {
            std::size_t n = p.size();
            if (n < 2) continue;
            for (std::size_t i = 0; i < n; ++i)
                total += p[i].distanceTo(p[(i + 1) % n]);
        }
        return total;
    }

    Geometry::GeometryType GeoRegion::type() const { return Geometry::GeometryType::Region; }

    std::unique_ptr<Geometry> GeoRegion::clone() const { return std::make_unique<GeoRegion>(*this); }

    void GeoRegion::reverse()
    {
        // 翻转所有部件的顶点顺序
        for (auto &p : parts_)
            std::reverse(p.begin(), p.end());
    }

    std::ostream &operator<<(std::ostream &os, const GeoRegion &r)
    {
        // 单部件沿用原格式，多部件逐部件输出
        if (r.parts_.size() == 1) {
            os << "GeoRegion[";
            const auto &p = r.parts_[0];
            for (std::size_t i = 0; i < p.size(); ++i) { if (i) os << ", "; os << p[i]; }
            return os << "]";
        }
        os << "MultiRegion[";
        for (std::size_t pi = 0; pi < r.parts_.size(); ++pi) {
            if (pi) os << ", ";
            os << "(";
            const auto &p = r.parts_[pi];
            for (std::size_t i = 0; i < p.size(); ++i) { if (i) os << ", "; os << p[i]; }
            os << ")";
        }
        return os << "]";
    }
} // namespace geo
