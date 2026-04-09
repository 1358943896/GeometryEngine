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

void GeometryWidget::setGeometries(std::vector<const geo::Geometry *> geometries){
    geometries_    = std::move(geometries);
    selectedIndex_ = -1;
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
    glClear(GL_COLOR_BUFFER_BIT);

    shader_.bind();
    shader_.setUniformValue(uViewCenter_, static_cast<float>(viewCx_),
                                          static_cast<float>(viewCy_));
    shader_.setUniformValue(uScale_,      static_cast<float>(scale_));
    shader_.setUniformValue(uViewport_,   static_cast<float>(width()),
                                          static_cast<float>(height()));
    shader_.setUniformValue(uPointSize_,  pointSize_);

    for (std::size_t i = 0; i < geometries_.size(); ++i)
        drawGeometry(geometries_[i], static_cast<int>(i) == selectedIndex_);

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
        for (std::size_t i = 0; i < geometries_.size(); ++i)
        {
            if (hitTest(geometries_[i], wx, wy))
            {
                selectedIndex_ = static_cast<int>(i);
                break;
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
        // 多边形：射线法判断点是否在内部
        const auto *region = static_cast<const geo::GeoRegion *>(geom);
        for (std::size_t p = 0; p < region->partCount(); ++p)
        {
            const auto        &verts = region->part(p);
            const std::size_t  n     = verts.size();
            if (n < 3) continue;

            bool inside = false;
            for (std::size_t i = 0, j = n - 1; i < n; j = i++)
            {
                const double xi = verts[i].x, yi = verts[i].y;
                const double xj = verts[j].x, yj = verts[j].y;
                if (((yi > wy) != (yj > wy)) &&
                    (wx < (xj - xi) * (wy - yi) / (yj - yi) + xi))
                    inside = !inside;
            }
            if (inside) return true;
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
            const auto &part = region->part(p);
            std::vector<float> pts;
            pts.reserve(part.size() * 2);
            for (const auto &v : part)
            {
                pts.push_back(static_cast<float>(v.x));
                pts.push_back(static_cast<float>(v.y));
            }
            // 半透明填充
            uploadAndDraw(pts, GL_TRIANGLE_FAN, cr, cg, cb, 0.2f);
            // 宽线轮廓
            auto strip = buildWideLineStrip(pts, true);
            uploadAndDraw(strip, GL_TRIANGLE_STRIP, cr, cg, cb, 1.0f);
        }
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
