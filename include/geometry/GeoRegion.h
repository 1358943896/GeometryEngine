#pragma once

#include "Geometry.h"
#include "../base/Coord.h"
#include <vector>
#include <iosfwd>
#include <memory>

namespace geo
{
    // 多边形：支持多部件，每个部件为一个带洞多边形
    // 数据结构遵循 OGC 标准：PartList 中每个元素是一个 RingList
    // RingList 的第一个元素是外环（exterior ring），后续元素是内环（interior rings/holes）
    class GeoRegion : public Geometry
    {
    public:
        using VertexList = std::vector<Coord>;
        using RingList   = std::vector<VertexList>;  // [0]=外环, [1..]=内环
        using PartList   = std::vector<RingList>;    // 多部件列表

        GeoRegion();

        //! \brief 构造单部件无洞多边形
        //! \param vertices 外环顶点列表
        explicit GeoRegion(VertexList vertices);

        GeoRegion(std::initializer_list<Coord> vertices);

        //! \brief 构造单部件带洞多边形
        //! \param rings 环列表，第一个是外环，后续是内环
        explicit GeoRegion(RingList rings);

        //! \brief 构造多部件多边形
        //! \param parts 各部件环列表
        explicit GeoRegion(PartList parts);

        //! \brief 返回部件数量
        std::size_t partCount() const;

        //! \brief 返回指定部件的外环（只读）
        //! \param i 部件索引
        const VertexList &exteriorRing(std::size_t i) const;

        //! \brief 返回指定部件的内环数量
        //! \param i 部件索引
        std::size_t interiorRingCount(std::size_t i) const;

        //! \brief 返回指定部件的指定内环（只读）
        //! \param partIdx 部件索引
        //! \param ringIdx 内环索引（0=第一个内环）
        const VertexList &interiorRing(std::size_t partIdx, std::size_t ringIdx) const;

        //! \brief 为指定部件添加内环（洞）
        //! \param i 部件索引
        //! \param ring 内环顶点列表
        void addInteriorRing(std::size_t i, VertexList ring);

        //! \brief 返回所有部件的边总数
        std::size_t edgeCount() const;

        // 迭代器：遍历各部件（RingList）
        PartList::const_iterator begin() const;
        PartList::const_iterator end() const;
        PartList::iterator begin();
        PartList::iterator end();

        BoundingBox boundingBox() const override;

        //! \brief 返回所有部件的顶点总数
        std::size_t vertexCount() const override;

        //! \brief 按扁平索引返回顶点
        //! \param i 顶点扁平索引
        const Coord &vertex(std::size_t i) const override;

        double area() const override;
        double perimeter() const override;
        Geometry::GeometryType type() const override;
        std::unique_ptr<Geometry> clone() const override;

        //! \brief 翻转所有部件的顶点顺序
        void reverse();

        friend std::ostream &operator<<(std::ostream &os, const GeoRegion &r);

    private:
        PartList parts_;
    };
} // namespace geo
