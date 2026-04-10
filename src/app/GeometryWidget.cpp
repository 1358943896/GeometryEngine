#include "GeometryWidget.h"
#include "base/BoundingBox.h"

#include <QWheelEvent>
#include <QMouseEvent>

#include <cmath>
#include <limits>

// ── Shader 源码 ───────────────────────────────────────────────────────────────

static const char *kVertSrc = R"(
#version 150 core
in vec2 pos;
uniform vec2  viewCenter;
uniform float scale;
uniform vec2  viewport;
uniform float pointSize;
void main() {
    vec2 ndc     = (pos - viewCenter) * scale / (viewport * 0.5);
    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = pointSize;
}
)";

static const char *kFragSrc = R"(
#version 150 core
uniform vec4 color;
uniform bool isPoint;
out vec4 fragColor;
void main() {
    // 点绘制时用 gl_PointCoord 裁剪为圆形
    if (isPoint) {
        vec2 c = gl_PointCoord - vec2(0.5);
        if (dot(c, c) > 0.25) discard;
    }
    fragColor = color;
}
)";

// ── 构造 / 析构 ───────────────────────────────────────────────────────────────

GeometryWidget::GeometryWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
}

GeometryWidget::~GeometryWidget()
{
    makeCurrent();
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    doneCurrent();
}

// ── 公共接口 ──────────────────────────────────────────────────────────────────

void GeometryWidget::setLayers(std::vector<Layer *> layers)
{
    layers_ = std::move(layers);
    geometries_.clear();
    selectedIndex_ = -1;
    selectedLayerIndex_ = -1;
    fitView();
    update();
}

void GeometryWidget::setGeometries(std::vector<const geo::Geometry *> geometries)
{
    geometries_ = std::move(geometries);
    layers_.clear();
    selectedIndex_ = -1;
    selectedLayerIndex_ = -1;
    fitView();
    update();
}

void GeometryWidget::setPointSize(int size)
{
    pointSize_ = static_cast<float>(size);
    update();
}

void GeometryWidget::setLineWidth(int width)
{
    lineWidth_ = static_cast<float>(width);
    update();
}

// ── OpenGL 生命周期 ───────────────────────────────────────────────────────────

void GeometryWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);  // 启用 stencil buffer 用于岛洞多边形绘制

    // 编译链接 shader，绑定 attrib location
    shader_.addShaderFromSourceCode(QOpenGLShader::Vertex,   kVertSrc);
    shader_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
    shader_.bindAttributeLocation("pos", 0);
    shader_.link();

    uViewCenter_ = shader_.uniformLocation("viewCenter");
    uScale_      = shader_.uniformLocation("scale");
    uViewport_   = shader_.uniformLocation("viewport");
    uColor_      = shader_.uniformLocation("color");
    uPointSize_  = shader_.uniformLocation("pointSize");
    uIsPoint_    = shader_.uniformLocation("isPoint");

    // 创建 VAO / VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

void GeometryWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GeometryWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);  // 同时清除 stencil buffer

    shader_.bind();
    shader_.setUniformValue(uViewCenter_, static_cast<float>(viewCx_),
                                          static_cast<float>(viewCy_));
    shader_.setUniformValue(uScale_,      static_cast<float>(scale_));
    shader_.setUniformValue(uViewport_,   static_cast<float>(width()),
                                          static_cast<float>(height()));

    // 优先使用图层模式绘制
    if (!layers_.empty())
    {
        // 先绘制所有未选中的图层
        for (std::size_t li = 0; li < layers_.size(); ++li)
        {
            if (static_cast<int>(li) == selectedLayerIndex_) continue;  // 跳过选中的图层

            Layer *layer = layers_[li];
            if (!layer || !layer->isVisible()) continue;

            shader_.setUniformValue(uPointSize_, layer->pointStyle().size);

            const auto &geoms = layer->geometries();
            for (std::size_t gi = 0; gi < geoms.size(); ++gi)
                drawGeometry(geoms[gi].get(), layer, false);
        }

        // 最后绘制选中的图层（在最上层显示选中效果）
        if (selectedLayerIndex_ >= 0 && selectedLayerIndex_ < static_cast<int>(layers_.size()))
        {
            Layer *selectedLayer = layers_[selectedLayerIndex_];
            if (selectedLayer && selectedLayer->isVisible())
            {
                shader_.setUniformValue(uPointSize_, selectedLayer->pointStyle().size);

                const auto &geoms = selectedLayer->geometries();
                for (std::size_t gi = 0; gi < geoms.size(); ++gi)
                    drawGeometry(geoms[gi].get(), selectedLayer, true);
            }
        }
    }
    else
    {
        // 兼容旧接口：使用全局样式
        shader_.setUniformValue(uPointSize_, pointSize_);
        for (std::size_t i = 0; i < geometries_.size(); ++i)
            drawGeometry(geometries_[i], static_cast<int>(i) == selectedIndex_);
    }

    shader_.release();
}

// ── 鼠标 / 滚轮事件 ───────────────────────────────────────────────────────────

void GeometryWidget::wheelEvent(QWheelEvent *event)
{
    // 以鼠标位置为锚点缩放，保持鼠标下的世界坐标不变
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const QPointF pos   = event->position();

    auto [wx, wy] = screenToWorld(pos.x(), pos.y());
    scale_ *= factor;
    viewCx_ = wx - (pos.x() - width()  * 0.5) / scale_;
    viewCy_ = wy + (pos.y() - height() * 0.5) / scale_;

    update();
}

void GeometryWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragging_    = true;
        dragStart_   = event->pos();
        dragStartCx_ = viewCx_;
        dragStartCy_ = viewCy_;
    }
}

void GeometryWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!dragging_) return;

    // 将像素偏移转换为世界坐标偏移
    const double dx = (event->pos().x() - dragStart_.x()) / scale_;
    const double dy = (event->pos().y() - dragStart_.y()) / scale_;
    viewCx_ = dragStartCx_ - dx;
    viewCy_ = dragStartCy_ + dy;

    update();
}

void GeometryWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    const QPoint delta = event->pos() - dragStart_;
    dragging_ = false;

    // 移动距离小于阈值视为点击，执行点选
    if (delta.manhattanLength() < 4)
    {
        auto [wx, wy] = screenToWorld(event->pos().x(), event->pos().y());
        selectedIndex_ = -1;
        selectedLayerIndex_ = -1;

        // 图层模式：从最上层（列表末尾）开始检测，只选中第一个命中的图层
        if (!layers_.empty())
        {
            // 反向遍历，从最上层开始
            for (int li = static_cast<int>(layers_.size()) - 1; li >= 0; --li)
            {
                Layer *layer = layers_[li];
                if (!layer || !layer->isVisible()) continue;

                const auto &geoms = layer->geometries();
                for (std::size_t gi = 0; gi < geoms.size(); ++gi)
                {
                    if (hitTest(geoms[gi].get(), wx, wy))
                    {
                        selectedLayerIndex_ = li;
                        break;
                    }
                }
                if (selectedLayerIndex_ >= 0) break;  // 找到最上层的命中图层后停止
            }
        }
        else
        {
            // 兼容旧接口
            for (std::size_t i = 0; i < geometries_.size(); ++i)
            {
                if (hitTest(geometries_[i], wx, wy))
                {
                    selectedIndex_ = static_cast<int>(i);
                    break;
                }
            }
        }
        update();
    }
}

// ── 私有辅助 ──────────────────────────────────────────────────────────────────

std::pair<double, double> GeometryWidget::screenToWorld(double sx, double sy) const
{
    const double wx =  (sx - width()  * 0.5) / scale_ + viewCx_;
    const double wy = -(sy - height() * 0.5) / scale_ + viewCy_;
    return {wx, wy};
}

bool GeometryWidget::hitTest(const geo::Geometry *geom, double wx, double wy) const
{
    const double thresh = kHitRadius / scale_;
    using Type = geo::Geometry::GeometryType;

    if (geom->type() == Type::Point)
    {
        // 点：检查每个部件与鼠标的距离
        for (std::size_t i = 0; i < geom->vertexCount(); ++i)
        {
            const auto &v = geom->vertex(i);
            const double dx = v.x - wx, dy = v.y - wy;
            if (std::sqrt(dx * dx + dy * dy) <= thresh)
                return true;
        }
    }
    else if (geom->type() == Type::Line)
    {
        // 折线：检查每条线段到鼠标的距离
        const auto *line = static_cast<const geo::GeoLine *>(geom);
        for (std::size_t p = 0; p < line->partCount(); ++p)
        {
            const auto &verts = line->part(p);
            for (std::size_t k = 1; k < verts.size(); ++k)
            {
                const geo::Coord &a = verts[k - 1];
                const geo::Coord &b = verts[k];
                const double abx  = b.x - a.x, aby = b.y - a.y;
                const double len2 = abx * abx + aby * aby;
                double dist;
                if (len2 < 1e-20)
                {
                    const double dx = wx - a.x, dy = wy - a.y;
                    dist = std::sqrt(dx * dx + dy * dy);
                }
                else
                {
                    const double t  = std::max(0.0, std::min(1.0,
                        ((wx - a.x) * abx + (wy - a.y) * aby) / len2));
                    const double px = a.x + t * abx - wx;
                    const double py = a.y + t * aby - wy;
                    dist = std::sqrt(px * px + py * py);
                }
                if (dist <= thresh) return true;
            }
        }
    }
    else // Region
    {
        // 带洞多边形：射线法判断点是否在内部
        // 点在外环内部且不在任一内环内部才算在多边形内
        const auto *region = static_cast<const geo::GeoRegion *>(geom);
        for (std::size_t p = 0; p < region->partCount(); ++p)
        {
            const auto &exterior = region->exteriorRing(p);
            const std::size_t n = exterior.size();
            if (n < 3) continue;

            // 射线法判断是否在外环内部
            bool insideExterior = false;
            for (std::size_t i = 0, j = n - 1; i < n; j = i++)
            {
                const double xi = exterior[i].x, yi = exterior[i].y;
                const double xj = exterior[j].x, yj = exterior[j].y;
                if (((yi > wy) != (yj > wy)) &&
                    (wx < (xj - xi) * (wy - yi) / (yj - yi) + xi))
                    insideExterior = !insideExterior;
            }

            if (!insideExterior) continue;  // 不在外环内，跳过此部件

            // 检查是否在任一内环内部（洞中）
            bool insideHole = false;
            for (std::size_t ri = 0; ri < region->interiorRingCount(p); ++ri)
            {
                const auto &interior = region->interiorRing(p, ri);
                const std::size_t m = interior.size();
                if (m < 3) continue;
                bool insideThisHole = false;
                for (std::size_t i = 0, j = m - 1; i < m; j = i++)
                {
                    const double xi = interior[i].x, yi = interior[i].y;
                    const double xj = interior[j].x, yj = interior[j].y;
                    if (((yi > wy) != (yj > wy)) &&
                        (wx < (xj - xi) * (wy - yi) / (yj - yi) + xi))
                        insideThisHole = !insideThisHole;
                }
                if (insideThisHole)
                {
                    insideHole = true;
                    break;
                }
            }

            // 在外环内且不在洞内，则点在多边形内
            if (!insideHole) return true;
        }
    }
    return false;
}

void GeometryWidget::uploadAndDraw(const std::vector<float> &verts, GLenum mode,
                                   float r, float g, float b, float a)
{
    if (verts.empty()) return;

    // 上传顶点数据到 VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STREAM_DRAW);

    shader_.setUniformValue(uColor_, r, g, b, a);

    glBindVertexArray(vao_);
    glDrawArrays(mode, 0, static_cast<GLsizei>(verts.size() / 2));
    glBindVertexArray(0);
}

void GeometryWidget::drawGeometry(const geo::Geometry *geom, bool selected)
{
    using Type = geo::Geometry::GeometryType;

    const float cr = selected ? 1.0f : 1.0f;
    const float cg = selected ? 0.85f : 1.0f;
    const float cb = selected ? 0.0f : 1.0f;

    if (geom->type() == Type::Point)
    {
        // 圆形点：fragment shader 用 gl_PointCoord 裁剪
        shader_.setUniformValue(uIsPoint_, 1);
        std::vector<float> verts;
        verts.reserve(geom->vertexCount() * 2);
        for (std::size_t i = 0; i < geom->vertexCount(); ++i)
        {
            const auto &v = geom->vertex(i);
            verts.push_back(static_cast<float>(v.x));
            verts.push_back(static_cast<float>(v.y));
        }
        uploadAndDraw(verts, GL_POINTS, cr, cg, cb, 1.0f);
        shader_.setUniformValue(uIsPoint_, 0);
    }
    else if (geom->type() == Type::Line)
    {
        // 宽线：CPU 生成 TRIANGLE_STRIP 四边形带
        const auto *line = static_cast<const geo::GeoLine *>(geom);
        for (std::size_t p = 0; p < line->partCount(); ++p)
        {
            const auto &part = line->part(p);
            std::vector<float> pts;
            pts.reserve(part.size() * 2);
            for (const auto &v : part)
            {
                pts.push_back(static_cast<float>(v.x));
                pts.push_back(static_cast<float>(v.y));
            }
            auto strip = buildWideLineStrip(pts, false);
            uploadAndDraw(strip, GL_TRIANGLE_STRIP, cr, cg, cb, 1.0f);
        }
    }
    else // Region
    {
        const auto *region = static_cast<const geo::GeoRegion *>(geom);
        for (std::size_t p = 0; p < region->partCount(); ++p)
        {
            const auto &exterior = region->exteriorRing(p);
            const std::size_t interiorCount = region->interiorRingCount(p);

            // 准备外环顶点数据
            std::vector<float> exteriorPts;
            exteriorPts.reserve(exterior.size() * 2);
            for (const auto &v : exterior)
            {
                exteriorPts.push_back(static_cast<float>(v.x));
                exteriorPts.push_back(static_cast<float>(v.y));
            }

            // 准备所有内环顶点数据
            std::vector<std::vector<float>> interiorPtsList;
            for (std::size_t ri = 0; ri < interiorCount; ++ri)
            {
                const auto &interior = region->interiorRing(p, ri);
                std::vector<float> interiorPts;
                interiorPts.reserve(interior.size() * 2);
                for (const auto &v : interior)
                {
                    interiorPts.push_back(static_cast<float>(v.x));
                    interiorPts.push_back(static_cast<float>(v.y));
                }
                interiorPtsList.push_back(std::move(interiorPts));
            }

            if (interiorCount == 0)
            {
                // 无洞：直接填充和绘制轮廓
                uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, cr, cg, cb, 0.2f);
                auto strip = buildWideLineStrip(exteriorPts, true);
                uploadAndDraw(strip, GL_TRIANGLE_STRIP, cr, cg, cb, 1.0f);
            }
            else
            {
                // 有洞：使用 stencil buffer 实现镂空填充（Even-Odd 规则）
                // Step 1: 设置 stencil 操作（翻转位）
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glStencilFunc(GL_ALWAYS, 0, 0x01);
                glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

                // 绘制外环（翻转 stencil）
                uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, 0, 0, 0, 0);

                // 绘制内环（再次翻转，实现镂空）
                for (const auto &interiorPts : interiorPtsList)
                    uploadAndDraw(interiorPts, GL_TRIANGLE_FAN, 0, 0, 0, 0);

                // Step 2: 在 stencil = 1 的区域填充颜色
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glStencilFunc(GL_EQUAL, 0x01, 0x01);
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, cr, cg, cb, 0.3f);

                // Step 3: 恢复 stencil 状态，绘制轮廓
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                auto strip = buildWideLineStrip(exteriorPts, true);
                uploadAndDraw(strip, GL_TRIANGLE_STRIP, cr, cg, cb, 1.0f);
                for (const auto &interiorPts : interiorPtsList)
                {
                    auto interiorStrip = buildWideLineStrip(interiorPts, true);
                    uploadAndDraw(interiorStrip, GL_TRIANGLE_STRIP, cr, cg, cb, 1.0f);
                }
            }
        }
    }
}

void GeometryWidget::drawGeometry(const geo::Geometry *geom, const Layer *layer, bool selected)
{
    if (!geom || !layer) return;

    using Type = geo::Geometry::GeometryType;

    // 根据几何类型获取图层样式
    const PointStyle &ps = layer->pointStyle();
    const LineStyle &ls = layer->lineStyle();
    const RegionStyle &rs = layer->regionStyle();

    // 选中时显示高亮颜色
    auto toFloat = [](const QColor &c) { return std::make_tuple(c.redF(), c.greenF(), c.blueF(), c.alphaF()); };

    if (geom->type() == Type::Point)
    {
        // 点：使用图层点样式
        auto [r, g, b, a] = toFloat(selected ? QColor(255, 217, 0) : ps.color);

        shader_.setUniformValue(uIsPoint_, 1);
        std::vector<float> verts;
        verts.reserve(geom->vertexCount() * 2);
        for (std::size_t i = 0; i < geom->vertexCount(); ++i)
        {
            const auto &v = geom->vertex(i);
            verts.push_back(static_cast<float>(v.x));
            verts.push_back(static_cast<float>(v.y));
        }
        uploadAndDraw(verts, GL_POINTS, r, g, b, a);
        shader_.setUniformValue(uIsPoint_, 0);
    }
    else if (geom->type() == Type::Line)
    {
        // 线：使用图层线样式
        auto [r, g, b, a] = toFloat(selected ? QColor(255, 217, 0) : ls.color);

        // 使用图层线宽临时更新 lineWidth_
        float savedLineWidth = lineWidth_;
        lineWidth_ = ls.width;

        const auto *line = static_cast<const geo::GeoLine *>(geom);
        for (std::size_t p = 0; p < line->partCount(); ++p)
        {
            const auto &part = line->part(p);
            std::vector<float> pts;
            pts.reserve(part.size() * 2);
            for (const auto &v : part)
            {
                pts.push_back(static_cast<float>(v.x));
                pts.push_back(static_cast<float>(v.y));
            }
            auto strip = buildWideLineStrip(pts, false);
            uploadAndDraw(strip, GL_TRIANGLE_STRIP, r, g, b, a);
        }

        lineWidth_ = savedLineWidth;
    }
    else // Region
    {
        // 面：使用图层面样式
        auto [lr, lg, lb, la] = toFloat(selected ? QColor(255, 217, 0) : rs.lineColor);
        auto [fr, fg, fb, fa] = toFloat(rs.fillColor);

        // 调试：确保填充颜色可见
        // 如果 fillColor 的 alpha 太低，使用默认可见颜色
        if (fa < 0.1f)
        {
            fr = 0.4f; fg = 0.7f; fb = 1.0f; fa = 0.7f;
        }

        // 使用图层线宽临时更新 lineWidth_
        float savedLineWidth = lineWidth_;
        lineWidth_ = rs.lineWidth;

        const auto *region = static_cast<const geo::GeoRegion *>(geom);
        for (std::size_t p = 0; p < region->partCount(); ++p)
        {
            const auto &exterior = region->exteriorRing(p);
            const std::size_t interiorCount = region->interiorRingCount(p);

            // 准备外环顶点数据
            std::vector<float> exteriorPts;
            exteriorPts.reserve(exterior.size() * 2);
            for (const auto &v : exterior)
            {
                exteriorPts.push_back(static_cast<float>(v.x));
                exteriorPts.push_back(static_cast<float>(v.y));
            }

            // 准备所有内环顶点数据
            std::vector<std::vector<float>> interiorPtsList;
            for (std::size_t ri = 0; ri < interiorCount; ++ri)
            {
                const auto &interior = region->interiorRing(p, ri);
                std::vector<float> interiorPts;
                interiorPts.reserve(interior.size() * 2);
                for (const auto &v : interior)
                {
                    interiorPts.push_back(static_cast<float>(v.x));
                    interiorPts.push_back(static_cast<float>(v.y));
                }
                interiorPtsList.push_back(std::move(interiorPts));
            }

            if (interiorCount == 0)
            {
                // 无洞：直接填充和绘制轮廓
                if (rs.showFill)
                    uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, fr, fg, fb, fa);
                auto strip = buildWideLineStrip(exteriorPts, true);
                uploadAndDraw(strip, GL_TRIANGLE_STRIP, lr, lg, lb, la);
            }
            else
            {
                // 有洞：使用 stencil buffer 实现镂空填充（Even-Odd 规则）
                if (rs.showFill)
                {
                    // Step 1: 启用 stencil 测试，设置 Even-Odd 规则
                    // 每次绘制翻转 stencil 的最低位
                    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                    glStencilFunc(GL_ALWAYS, 0, 0x01);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);  // 翻转 stencil 位

                    // 绘制外环（翻转 stencil）
                    uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, 0, 0, 0, 0);

                    // 绘制内环（再次翻转，实现镂空）
                    for (const auto &interiorPts : interiorPtsList)
                        uploadAndDraw(interiorPts, GL_TRIANGLE_FAN, 0, 0, 0, 0);

                    // Step 2: 在 stencil = 1 的区域绘制填充
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glStencilFunc(GL_EQUAL, 0x01, 0x01);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

                    // 使用外环边界绘制填充
                    uploadAndDraw(exteriorPts, GL_TRIANGLE_FAN, fr, fg, fb, fa);

                    // Step 3: 恢复 stencil 状态
                    glStencilFunc(GL_ALWAYS, 0, 0xFF);
                }

                // 绘制轮廓（外环和所有内环）
                auto strip = buildWideLineStrip(exteriorPts, true);
                uploadAndDraw(strip, GL_TRIANGLE_STRIP, lr, lg, lb, la);
                for (const auto &interiorPts : interiorPtsList)
                {
                    auto interiorStrip = buildWideLineStrip(interiorPts, true);
                    uploadAndDraw(interiorStrip, GL_TRIANGLE_STRIP, lr, lg, lb, la);
                }
            }
        }

        lineWidth_ = savedLineWidth;
    }
}

std::vector<float> GeometryWidget::buildWideLineStrip(const std::vector<float> &pts,
                                                       bool closed) const
{
    // 将折线顶点扩展为宽线四边形带（TRIANGLE_STRIP）
    // 法线方向在屏幕空间计算，半宽 = lineWidth_ / scale_ / 2
    const std::size_t n = pts.size() / 2;
    if (n < 2) return {};

    // 半宽转换为世界坐标
    const float hw = lineWidth_ * 0.5f / static_cast<float>(scale_);

    // 计算每个顶点处的法线（相邻线段法线平均）
    std::vector<float> nx(n), ny(n);
    auto segNorm = [&](std::size_t i, std::size_t j, float &ox, float &oy) {
        float dx = pts[j*2] - pts[i*2];
        float dy = pts[j*2+1] - pts[i*2+1];
        float len = std::sqrt(dx*dx + dy*dy);
        if (len < 1e-10f) { ox = 0; oy = 1; return; }
        ox = -dy / len;
        oy =  dx / len;
    };

    for (std::size_t i = 0; i < n; ++i)
    {
        float ax, ay, bx, by;
        if (i == 0)
        {
            if (closed) segNorm(n-1, 0, ax, ay); else segNorm(0, 1, ax, ay);
            segNorm(0, 1, bx, by);
        }
        else if (i == n-1)
        {
            segNorm(i-1, i, ax, ay);
            if (closed) segNorm(i, 0, bx, by); else { bx = ax; by = ay; }
        }
        else
        {
            segNorm(i-1, i, ax, ay);
            segNorm(i, i+1, bx, by);
        }
        // 平均法线
        float mx = ax + bx, my = ay + by;
        float ml = std::sqrt(mx*mx + my*my);
        if (ml < 1e-10f) { mx = ax; my = ay; }
        else { mx /= ml; my /= ml; }
        // miter 长度修正
        float dot = ax*mx + ay*my;
        float miter = (dot > 1e-4f) ? hw / dot : hw;
        nx[i] = mx * miter;
        ny[i] = my * miter;
    }

    // 生成 TRIANGLE_STRIP 顶点
    std::vector<float> strip;
    strip.reserve(n * 4 + (closed ? 4 : 0));
    for (std::size_t i = 0; i < n; ++i)
    {
        strip.push_back(pts[i*2]   + nx[i]);
        strip.push_back(pts[i*2+1] + ny[i]);
        strip.push_back(pts[i*2]   - nx[i]);
        strip.push_back(pts[i*2+1] - ny[i]);
    }
    // 闭合：重复第一对顶点
    if (closed)
    {
        strip.push_back(pts[0] + nx[0]);
        strip.push_back(pts[1] + ny[0]);
        strip.push_back(pts[0] - nx[0]);
        strip.push_back(pts[1] - ny[0]);
    }
    return strip;
}

void GeometryWidget::fitView(){
    // 优先使用图层模式
    if (!layers_.empty())
    {
        double minX =  std::numeric_limits<double>::max();
        double minY =  std::numeric_limits<double>::max();
        double maxX = -std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        bool any = false;

        for (const auto *layer : layers_)
        {
            if (!layer || !layer->isVisible()) continue;
            for (const auto &g : layer->geometries())
            {
                const auto bb = g->boundingBox();
                minX = std::min(minX, bb.left);
                minY = std::min(minY, bb.bottom);
                maxX = std::max(maxX, bb.right);
                maxY = std::max(maxY, bb.top);
                any = true;
            }
        }

        if (!any) return;

        viewCx_ = (minX + maxX) * 0.5;
        viewCy_ = (minY + maxY) * 0.5;

        const double ww   = width()  > 0 ? width()  : 800;
        const double wh   = height() > 0 ? height() : 600;
        const double extX = (maxX - minX) * 0.5 * 1.1;
        const double extY = (maxY - minY) * 0.5 * 1.1;

        if (extX > 1e-10 && extY > 1e-10)
            scale_ = std::min(ww * 0.5 / extX, wh * 0.5 / extY);
        else
            scale_ = 1.0;
        return;
    }

    // 兼容旧接口
    if (geometries_.empty()) return;

    double minX =  std::numeric_limits<double>::max();
    double minY =  std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();

    for (const auto *g : geometries_)
    {
        const auto bb = g->boundingBox();
        minX = std::min(minX, bb.left);
        minY = std::min(minY, bb.bottom);
        maxX = std::max(maxX, bb.right);
        maxY = std::max(maxY, bb.top);
    }

    viewCx_ = (minX + maxX) * 0.5;
    viewCy_ = (minY + maxY) * 0.5;

    const double ww   = width()  > 0 ? width()  : 800;
    const double wh   = height() > 0 ? height() : 600;
    const double extX = (maxX - minX) * 0.5 * 1.1;
    const double extY = (maxY - minY) * 0.5 * 1.1;

    if (extX > 1e-10 && extY > 1e-10)
        scale_ = std::min(ww * 0.5 / extX, wh * 0.5 / extY);
    else
        scale_ = 1.0;
}
