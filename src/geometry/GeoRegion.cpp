#include "geometry/GeoRegion.h"
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace geo
{
    GeoRegion::GeoRegion() = default;

    GeoRegion::GeoRegion(VertexList vertices) : parts_({RingList{std::move(vertices)}})
    {
    }

    GeoRegion::GeoRegion(std::initializer_list<Coord> vertices) : parts_({RingList{VertexList(vertices)}})
    {
    }

    GeoRegion::GeoRegion(RingList rings) : parts_({std::move(rings)})
    {
    }

    GeoRegion::GeoRegion(PartList parts) : parts_(std::move(parts))
    {
    }

    std::size_t GeoRegion::partCount() const
    {
        return parts_.size();
    }

    const GeoRegion::VertexList &GeoRegion::exteriorRing(std::size_t i) const
    {
        // 返回指定部件的外环（第一个环）
        return parts_.at(i).at(0);
    }

    std::size_t GeoRegion::interiorRingCount(std::size_t i) const
    {
        // 内环数量 = 总环数 - 1（外环）
        const RingList &rings = parts_.at(i);
        return rings.size() > 1 ? rings.size() - 1 : 0;
    }

    const GeoRegion::VertexList &GeoRegion::interiorRing(std::size_t partIdx, std::size_t ringIdx) const
    {
        // 内环索引从 0 开始，实际存储位置是 ringIdx + 1（跳过外环）
        const RingList &rings = parts_.at(partIdx);
        if (ringIdx + 1 >= rings.size())
            throw std::out_of_range("GeoRegion::interiorRing index out of range");
        return rings[ringIdx + 1];
    }

    void GeoRegion::addInteriorRing(std::size_t i, VertexList ring)
    {
        // 为指定部件添加内环
        parts_.at(i).push_back(std::move(ring));
    }

    std::size_t GeoRegion::edgeCount() const
    {
        // 每个环的边数等于其顶点数（闭合环）
        // 累计所有部件所有环的边数
        std::size_t total = 0;
        for (const auto &part : parts_) {
            for (const auto &ring : part)
                total += ring.size();
        }
        return total;
    }

    GeoRegion::PartList::const_iterator GeoRegion::begin() const { return parts_.begin(); }
    GeoRegion::PartList::const_iterator GeoRegion::end()   const { return parts_.end(); }
    GeoRegion::PartList::iterator       GeoRegion::begin()       { return parts_.begin(); }
    GeoRegion::PartList::iterator       GeoRegion::end()         { return parts_.end(); }

    BoundingBox GeoRegion::boundingBox() const
    {
        // 只考虑外环计算包围盒（内环在外环内部，不影响整体包围盒）
        double minX = std::numeric_limits<double>::max();
        double minY = minX, maxX = -minX, maxY = -minX;
        bool any = false;
        for (const auto &part : parts_) {
            if (part.empty()) continue;
            const auto &exterior = part[0];  // 外环
            for (const auto &v : exterior) {
                minX = std::min(minX, v.x); minY = std::min(minY, v.y);
                maxX = std::max(maxX, v.x); maxY = std::max(maxY, v.y);
                any = true;
            }
        }
        return any ? BoundingBox{minX, minY, maxX, maxY} : BoundingBox{0, 0, 0, 0};
    }

    std::size_t GeoRegion::vertexCount() const
    {
        // 累加所有部件所有环的顶点数
        std::size_t total = 0;
        for (const auto &part : parts_) {
            for (const auto &ring : part)
                total += ring.size();
        }
        return total;
    }

    const Coord &GeoRegion::vertex(std::size_t i) const
    {
        // 按扁平索引定位顶点：先遍历部件，再遍历环
        for (const auto &part : parts_) {
            for (const auto &ring : part) {
                if (i < ring.size()) return ring[i];
                i -= ring.size();
            }
        }
        throw std::out_of_range("GeoRegion::vertex index out of range");
    }

    double GeoRegion::area() const
    {
        // 计算带洞多边形面积：外环面积 - 内环面积
        // 使用 Shoelace 公式，顺时针为正面积，逆时针为负面积
        double total = 0.0;
        for (const auto &part : parts_) {
            if (part.empty()) continue;
            // 外环面积（取绝对值）
            const auto &exterior = part[0];
            std::size_t n = exterior.size();
            if (n < 3) continue;
            double exteriorArea = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                const Coord &a = exterior[i];
                const Coord &b = exterior[(i + 1) % n];
                exteriorArea += a.x * b.y - b.x * a.y;
            }
            total += std::abs(exteriorArea) * 0.5;

            // 减去内环面积
            for (std::size_t ri = 1; ri < part.size(); ++ri) {
                const auto &interior = part[ri];
                n = interior.size();
                if (n < 3) continue;
                double interiorArea = 0.0;
                for (std::size_t i = 0; i < n; ++i) {
                    const Coord &a = interior[i];
                    const Coord &b = interior[(i + 1) % n];
                    interiorArea += a.x * b.y - b.x * a.y;
                }
                total -= std::abs(interiorArea) * 0.5;
            }
        }
        return total;
    }

    double GeoRegion::perimeter() const
    {
        // 累加所有部件所有环的周长（包括内环边界）
        double total = 0.0;
        for (const auto &part : parts_) {
            for (const auto &ring : part) {
                std::size_t n = ring.size();
                if (n < 2) continue;
                for (std::size_t i = 0; i < n; ++i)
                    total += ring[i].distanceTo(ring[(i + 1) % n]);
            }
        }
        return total;
    }

    Geometry::GeometryType GeoRegion::type() const { return Geometry::GeometryType::Region; }

    std::unique_ptr<Geometry> GeoRegion::clone() const { return std::make_unique<GeoRegion>(*this); }

    void GeoRegion::reverse()
    {
        // 翻转所有部件所有环的顶点顺序
        for (auto &part : parts_) {
            for (auto &ring : part)
                std::reverse(ring.begin(), ring.end());
        }
    }

    std::ostream &operator<<(std::ostream &os, const GeoRegion &r)
    {
        // 输出格式：单部件沿用原格式，多部件逐部件输出
        // 带洞时显示环数量
        if (r.parts_.size() == 1) {
            const auto &part = r.parts_[0];
            if (part.size() == 1) {
                // 单部件无洞
                os << "GeoRegion[";
                const auto &ring = part[0];
                for (std::size_t i = 0; i < ring.size(); ++i) { if (i) os << ", "; os << ring[i]; }
                return os << "]";
            } else {
                // 单部件带洞
                os << "GeoRegion[exterior:";
                const auto &exterior = part[0];
                for (std::size_t i = 0; i < exterior.size(); ++i) { if (i) os << ", "; os << exterior[i]; }
                os << ", holes:" << (part.size() - 1) << "]";
                return os;
            }
        }
        // 多部件
        os << "MultiRegion[";
        for (std::size_t pi = 0; pi < r.parts_.size(); ++pi) {
            if (pi) os << ", ";
            const auto &part = r.parts_[pi];
            os << "(exterior:";
            const auto &exterior = part[0];
            for (std::size_t i = 0; i < exterior.size(); ++i) { if (i) os << ", "; os << exterior[i]; }
            os << ", holes:" << (part.size() - 1) << ")";
        }
        return os << "]";
    }
} // namespace geo
