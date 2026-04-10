#pragma once

#include "geometry/Geometry.h"
#include <QString>
#include <QColor>
#include <vector>
#include <memory>

//! \brief 点样式配置
struct PointStyle
{
    QColor color = QColor(255, 255, 255);  //!< 点颜色
    float size = 10.0f;                     //!< 点大小（像素）
};

//! \brief 线样式配置
struct LineStyle
{
    QColor color = QColor(255, 255, 255);  //!< 线颜色
    float width = 2.0f;                     //!< 线宽（像素）
};

//! \brief 面样式配置
struct RegionStyle
{
    QColor lineColor = QColor(255, 255, 255);       //!< 边界线颜色
    float lineWidth = 2.0f;                         //!< 边界线宽（像素）
    QColor fillColor = QColor(100, 180, 255, 180);  //!< 填充颜色（默认半透明蓝色，alpha=180）
    bool showFill = true;                           //!< 是否显示填充
};

//! \brief 图层类，存储几何对象及其样式
class Layer
{
public:
    //! \brief 构造函数
    //! \param name 图层名称
    //! \param geometries 几何对象列表
    Layer(QString name, std::vector<std::unique_ptr<geo::Geometry>> geometries);

    //! \brief 返回图层名称
    const QString &name() const;

    //! \brief 设置图层名称
    //! \param name 新名称
    void setName(const QString &name);

    //! \brief 返回可见性
    bool isVisible() const;

    //! \brief 设置可见性
    //! \param visible 是否可见
    void setVisible(bool visible);

    //! \brief 根据图层中几何类型判断主要类型
    //! \return 第一个几何对象的类型，如果没有几何返回 Region
    geo::Geometry::GeometryType primaryType() const;

    //! \brief 返回点样式（可写）
    PointStyle &pointStyle();

    //! \brief 返回点样式（只读）
    const PointStyle &pointStyle() const;

    //! \brief 返回线样式（可写）
    LineStyle &lineStyle();

    //! \brief 返回线样式（只读）
    const LineStyle &lineStyle() const;

    //! \brief 返回面样式（可写）
    RegionStyle &regionStyle();

    //! \brief 返回面样式（只读）
    const RegionStyle &regionStyle() const;

    //! \brief 返回几何对象列表（只读）
    const std::vector<std::unique_ptr<geo::Geometry>> &geometries() const;

    //! \brief 返回几何对象指针列表（用于传递给绘制控件）
    std::vector<const geo::Geometry *> geometryPtrs() const;

private:
    QString name_;                                  //!< 图层名称
    std::vector<std::unique_ptr<geo::Geometry>> geometries_;  //!< 几何对象
    bool visible_ = true;                           //!< 可见性
    PointStyle pointStyle_;                         //!< 点样式
    LineStyle lineStyle_;                           //!< 线样式
    RegionStyle regionStyle_;                       //!< 面样式
};