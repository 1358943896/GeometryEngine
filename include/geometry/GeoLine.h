#pragma once

#include "Geometry.h"
#include "../base/Coord.h"
#include <vector>
#include <iosfwd>

namespace geo
{
    // 折线：支持多部件，每个部件为一条独立折线
    class GeoLine : public Geometry
    {
    public:
        using VertexList = std::vector<Coord>;
        using PartList   = std::vector<VertexList>;

        GeoLine();

        //! \brief 构造单部件折线
        //! \param vertices 顶点列表
        explicit GeoLine(VertexList vertices);

        GeoLine(std::initializer_list<Coord> vertices);

        //! \brief 两点构造单部件折线
        //! \param a 起点
        //! \param b 终点
        GeoLine(const Coord &a, const Coord &b);

        //! \brief 构造多部件折线
        //! \param parts 各部件顶点列表
        explicit GeoLine(PartList parts);

        //! \brief 返回部件数量
        std::size_t partCount() const;

        //! \brief 返回指定部件的顶点列表（只读）
        //! \param i 部件索引
        const VertexList &part(std::size_t i) const;

        //! \brief 返回指定部件的顶点列表（可写）
        //! \param i 部件索引
        VertexList &part(std::size_t i);

        //! \brief 添加一个新部件
        //! \param vertices 新部件顶点列表
        void addPart(VertexList vertices);

        //! \brief 返回所有部件的线段总数
        std::size_t segmentCount() const;

        //! \brief 返回第 i 条线段（跨部件扁平索引，不跨部件边界）
        //! \param i 线段扁平索引
        std::pair<Coord, Coord> segment(std::size_t i) const;

        // 迭代器：遍历各部件（VertexList）
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

        friend std::ostream &operator<<(std::ostream &os, const GeoLine &l);

    private:
        PartList parts_;
    };
} // namespace geo
