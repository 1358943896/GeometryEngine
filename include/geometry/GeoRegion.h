#pragma once

#include "Geometry.h"
#include "../base/Coord.h"
#include <vector>
#include <iosfwd>
#include <memory>

namespace geo
{
    // 多边形：支持多部件，每个部件为一个独立多边形环
    class GeoRegion : public Geometry
    {
    public:
        using VertexList = std::vector<Coord>;
        using PartList   = std::vector<VertexList>;

        GeoRegion();

        //! \brief 构造单部件多边形
        //! \param vertices 顶点列表
        explicit GeoRegion(VertexList vertices);

        GeoRegion(std::initializer_list<Coord> vertices);

        //! \brief 构造多部件多边形
        //! \param parts 各部件顶点列表
        explicit GeoRegion(PartList parts);

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

        //! \brief 返回所有部件的边总数
        std::size_t edgeCount() const;

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

        //! \brief 翻转所有部件的顶点顺序
        void reverse();

        friend std::ostream &operator<<(std::ostream &os, const GeoRegion &r);

    private:
        PartList parts_;
    };
} // namespace geo
