#include "geometry/GeoLine.h"
#include <cmath>
#include <ostream>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace geo
{
    GeoLine::GeoLine() = default;

    GeoLine::GeoLine(VertexList vertices) : parts_({std::move(vertices)})
    {
    }

    GeoLine::GeoLine(std::initializer_list<Coord> vertices) : parts_({VertexList(vertices)})
    {
    }

    GeoLine::GeoLine(const Coord &a, const Coord &b) : parts_({{a, b}})
    {
    }

    GeoLine::GeoLine(PartList parts) : parts_(std::move(parts))
    {
    }

    std::size_t GeoLine::partCount() const
    {
        return parts_.size();
    }

    const GeoLine::VertexList &GeoLine::part(std::size_t i) const
    {
        return parts_.at(i);
    }

    GeoLine::VertexList &GeoLine::part(std::size_t i)
    {
        return parts_.at(i);
    }

    void GeoLine::addPart(VertexList vertices)
    {
        // 追加一个新的折线部件
        parts_.push_back(std::move(vertices));
    }

    std::size_t GeoLine::segmentCount() const
    {
        // 累加每个部件的线段数（n 个顶点有 n-1 条线段）
        std::size_t count = 0;
        for (const auto &p : parts_)
            if (p.size() >= 2) count += p.size() - 1;
        return count;
    }

    std::pair<Coord, Coord> GeoLine::segment(std::size_t i) const
    {
        // 按扁平索引定位线段所在部件及局部索引
        for (const auto &p : parts_) {
            if (p.size() < 2) continue;
            std::size_t cnt = p.size() - 1;
            if (i < cnt) return {p[i], p[i + 1]};
            i -= cnt;
        }
        throw std::out_of_range("GeoLine::segment index out of range");
    }

    GeoLine::PartList::const_iterator GeoLine::begin() const { return parts_.begin(); }
    GeoLine::PartList::const_iterator GeoLine::end()   const { return parts_.end(); }
    GeoLine::PartList::iterator       GeoLine::begin()       { return parts_.begin(); }
    GeoLine::PartList::iterator       GeoLine::end()         { return parts_.end(); }

    BoundingBox GeoLine::boundingBox() const
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

    std::size_t GeoLine::vertexCount() const
    {
        // 累加所有部件的顶点数
        std::size_t total = 0;
        for (const auto &p : parts_) total += p.size();
        return total;
    }

    const Coord &GeoLine::vertex(std::size_t i) const
    {
        // 按扁平索引定位顶点所在部件
        for (const auto &p : parts_) {
            if (i < p.size()) return p[i];
            i -= p.size();
        }
        throw std::out_of_range("GeoLine::vertex index out of range");
    }

    double GeoLine::area() const { return 0.0; }

    double GeoLine::perimeter() const
    {
        // 累加所有部件的长度
        double total = 0.0;
        for (const auto &p : parts_)
            for (std::size_t i = 0; i + 1 < p.size(); ++i)
                total += p[i].distanceTo(p[i + 1]);
        return total;
    }

    Geometry::GeometryType GeoLine::type() const { return Geometry::GeometryType::Line; }

    std::unique_ptr<Geometry> GeoLine::clone() const { return std::make_unique<GeoLine>(*this); }

    std::ostream &operator<<(std::ostream &os, const GeoLine &l)
    {
        // 单部件沿用原格式，多部件逐部件输出
        if (l.parts_.size() == 1) {
            os << "GeoLine[";
            const auto &p = l.parts_[0];
            for (std::size_t i = 0; i < p.size(); ++i) { if (i) os << " -> "; os << p[i]; }
            return os << "]";
        }
        os << "MultiLine[";
        for (std::size_t pi = 0; pi < l.parts_.size(); ++pi) {
            if (pi) os << ", ";
            os << "(";
            const auto &p = l.parts_[pi];
            for (std::size_t i = 0; i < p.size(); ++i) { if (i) os << " -> "; os << p[i]; }
            os << ")";
        }
        return os << "]";
    }
} // namespace geo
