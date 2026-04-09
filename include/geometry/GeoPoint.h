#pragma once

#include "Geometry.h"
#include "../base/Coord.h"
#include <vector>
#include <iosfwd>
#include <memory>

namespace geo
{
    class GeoPoint : public Geometry
    {
    public:
        using PartList = std::vector<Coord>;

        //! \brief 构造单部件点
        //! \param coord 点坐标
        explicit GeoPoint(const Coord &coord);

        //! \brief 构造单部件点
        //! \param x x 坐标
        //! \param y y 坐标
        GeoPoint(double x, double y);

        //! \brief 构造多部件点
        //! \param coords 所有部件坐标列表
        explicit GeoPoint(PartList coords);

        //! \brief 返回部件数量
        std::size_t partCount() const;

        //! \brief 返回指定部件的坐标（只读）
        //! \param i 部件索引
        const Coord &part(std::size_t i) const;

        //! \brief 添加一个新部件
        //! \param coord 新部件坐标
        void addPart(const Coord &coord);

        //! \brief 返回第一个部件坐标（单部件兼容接口）
        const Coord &coord() const;

        //! \brief 返回第一个部件的 x 坐标（单部件兼容接口）
        double x() const;

        //! \brief 返回第一个部件的 y 坐标（单部件兼容接口）
        double y() const;

        PartList::const_iterator begin() const;
        PartList::const_iterator end() const;
        PartList::iterator begin();
        PartList::iterator end();

        BoundingBox boundingBox() const override;

        //! \brief 返回所有部件的顶点总数（等于部件数）
        std::size_t vertexCount() const override;

        //! \brief 返回第 i 个部件的坐标
        //! \param i 顶点（部件）索引
        const Coord &vertex(std::size_t i) const override;

        double area() const override;
        double perimeter() const override;

        Geometry::GeometryType type() const override;

        std::unique_ptr<Geometry> clone() const override;

        bool operator==(const GeoPoint &rhs) const;
        bool operator!=(const GeoPoint &rhs) const;

        friend std::ostream &operator<<(std::ostream &os, const GeoPoint &p);

    private:
        PartList coords_;
    };
} // namespace geo
