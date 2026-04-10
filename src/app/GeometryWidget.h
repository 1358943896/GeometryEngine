#pragma once

#include "geometry/GeoLine.h"
#include "geometry/GeoPoint.h"
#include "geometry/GeoRegion.h"
#include "geometry/Geometry.h"
#include "app/Layer.h"
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <vector>

//! \brief 基于 QOpenGLWidget 的几何图形绘制控件（现代管线）
//! 支持缩放（滚轮）、平移（左键拖拽）、点击选中（整体多部件选中）
//! 支持按图层样式绘制
class GeometryWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
    Q_OBJECT

public:
    explicit GeometryWidget(QWidget *parent = nullptr);
    ~GeometryWidget() override;

    //! \brief 设置要绘制的图层列表
    //! \param layers 图层指针列表（不拥有所有权）
    void setLayers(std::vector<Layer *> layers);

    //! \brief 设置要绘制的几何对象列表（不取所有权）- 兼容旧接口
    //! \param geometries 指向 Geometry 的指针列表
    void setGeometries(std::vector<const geo::Geometry *> geometries);

    //! \brief 设置点的显示大小（像素）
    //! \param size 点大小
    void setPointSize(int size);

    //! \brief 设置线宽（像素）
    //! \param width 线宽
    void setLineWidth(int width);

protected:
    //! \brief 初始化 OpenGL：编译 shader，创建 VAO/VBO
    void initializeGL() override;

    //! \brief 响应窗口尺寸变化，更新视口
    //! \param w 新宽度
    //! \param h 新高度
    void resizeGL(int w, int h) override;

    //! \brief 执行 OpenGL 绘制
    void paintGL() override;

    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    //! \brief 将屏幕像素坐标转换为世界坐标
    //! \param sx 屏幕 x（像素）
    //! \param sy 屏幕 y（像素）
    //! \return 世界坐标 (wx, wy)
    std::pair<double, double> screenToWorld(double sx, double sy) const;

    //! \brief 判断世界坐标点是否命中指定几何对象
    //! \param geom 目标几何对象
    //! \param wx 世界 x
    //! \param wy 世界 y
    bool hitTest(const geo::Geometry *geom, double wx, double wy) const;

    //! \brief 将折线顶点序列扩展为宽线四边形带（TRIANGLE_STRIP）
    //! \param pts    折线顶点（世界坐标，x/y 交替）
    //! \param closed 是否闭合（LINE_LOOP）
    //! \return TRIANGLE_STRIP 顶点数组
    std::vector<float> buildWideLineStrip(const std::vector<float> &pts,
                                          bool closed) const;

    //! \brief 上传顶点数据并绘制一次 draw call
    //! \param verts 顶点数组（x,y 交替）
    //! \param mode  GL 图元类型
    //! \param color RGBA 颜色
    void uploadAndDraw(const std::vector<float> &verts, GLenum mode,
                       float r, float g, float b, float a);

    //! \brief 绘制单个几何对象（使用全局样式）
    //! \param geom     目标几何对象
    //! \param selected 是否高亮选中色
    void drawGeometry(const geo::Geometry *geom, bool selected);

    //! \brief 绘制单个几何对象（使用图层样式）
    //! \param geom     目标几何对象
    //! \param layer    图层（提供样式）
    //! \param selected 是否高亮选中色
    void drawGeometry(const geo::Geometry *geom, const Layer *layer, bool selected);

    //! \brief 将所有几何对象的包围盒合并，用于初始化视图
    void fitView();

    std::vector<const geo::Geometry *> geometries_;  //!< 兼容旧接口的几何列表
    std::vector<Layer *> layers_;                    //!< 图层列表
    int selectedIndex_ = -1;
    int selectedLayerIndex_ = -1;  //!< 选中的图层索引

    // 视图变换参数
    double scale_  = 1.0;
    double viewCx_ = 0.0;
    double viewCy_ = 0.0;

    // 拖拽状态
    bool   dragging_    = false;
    QPoint dragStart_;
    double dragStartCx_ = 0.0;
    double dragStartCy_ = 0.0;

    // 现代管线资源
    QOpenGLShaderProgram shader_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    // uniform locations
    int uViewCenter_  = -1;
    int uScale_       = -1;
    int uViewport_    = -1;
    int uColor_       = -1;
    int uPointSize_   = -1;
    int uIsPoint_     = -1;

    static constexpr double kHitRadius = 8.0;

    float pointSize_ = 10.0f;
    float lineWidth_ = 1.5f;
};
