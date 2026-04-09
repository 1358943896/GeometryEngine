#pragma once

#include "base/BoundingBox.h"
#include "base/Coord.h"
#include <memory>


namespace geo
{
    // 所有几何对象的抽象基类
    class Geometry
    {
    public:
        virtual ~Geometry() = default;

        // 几何类型标签，用于轻量级运行时类型判断
        enum class GeometryType { Point, Line, Region };

        // 返回包围盒
        virtual BoundingBox boundingBox() const = 0;

        virtual std::size_t vertexCount() const = 0;

        virtual const Coord &vertex(std::size_t i) const = 0;

        // 面积（点和折线返回 0）
        virtual double area() const = 0;

        // 周长 / 总长度
        virtual double perimeter() const = 0;

        // 返回几何类型
        virtual GeometryType type() const = 0;

        // 深拷贝
        virtual std::unique_ptr<Geometry> clone() const = 0;
    };
} // namespace geo
