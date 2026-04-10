#include "app/Layer.h"

Layer::Layer(QString name, std::vector<std::unique_ptr<geo::Geometry>> geometries)
    : name_(std::move(name))
    , geometries_(std::move(geometries))
{
}

const QString &Layer::name() const
{
    return name_;
}

void Layer::setName(const QString &name)
{
    name_ = name;
}

bool Layer::isVisible() const
{
    return visible_;
}

void Layer::setVisible(bool visible)
{
    visible_ = visible;
}

geo::Geometry::GeometryType Layer::primaryType() const
{
    // 返回第一个几何对象的类型
    if (geometries_.empty())
        return geo::Geometry::GeometryType::Region;
    return geometries_[0]->type();
}

PointStyle &Layer::pointStyle()
{
    return pointStyle_;
}

const PointStyle &Layer::pointStyle() const
{
    return pointStyle_;
}

LineStyle &Layer::lineStyle()
{
    return lineStyle_;
}

const LineStyle &Layer::lineStyle() const
{
    return lineStyle_;
}

RegionStyle &Layer::regionStyle()
{
    return regionStyle_;
}

const RegionStyle &Layer::regionStyle() const
{
    return regionStyle_;
}

const std::vector<std::unique_ptr<geo::Geometry>> &Layer::geometries() const
{
    return geometries_;
}

std::vector<const geo::Geometry *> Layer::geometryPtrs() const
{
    std::vector<const geo::Geometry *> ptrs;
    ptrs.reserve(geometries_.size());
    for (const auto &g : geometries_)
        ptrs.push_back(g.get());
    return ptrs;
}