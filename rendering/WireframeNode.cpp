#include "WireframeNode.h"
#include <QSGFlatColorMaterial>
#include <cmath>
#include <vector>

WireframeNode::WireframeNode()
{
    auto *mesh = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
    mesh->setDrawingMode(QSGGeometry::DrawTriangles);
    mesh->setVertexDataPattern(QSGGeometry::StaticPattern);
    setGeometry(mesh);
    setFlag(OwnsGeometry);
    setMaterial(new QSGFlatColorMaterial);
    setFlag(OwnsMaterial);
}

void WireframeNode::setPath(const QPainterPath &path, qreal width)
{
    std::vector<QPointF> vertices;
    // Flatten curves once per preview, never on an animation frame. Each straight
    // segment uses two triangles regardless of how far apart its endpoints are.
    for (const auto &polygon : path.toSubpathPolygons()) {
        for (qsizetype i = 1; i < polygon.size(); ++i) {
            const QPointF a = polygon[i - 1];
            const QPointF b = polygon[i];
            const QPointF direction = b - a;
            const qreal length = std::hypot(direction.x(), direction.y());
            if (length <= 0 || !std::isfinite(length)) continue;
            const QPointF normal(-direction.y() * width / (2 * length),
                                  direction.x() * width / (2 * length));
            vertices.insert(vertices.end(), {a + normal, a - normal, b + normal,
                                             b + normal, a - normal, b - normal});
        }
    }
    auto *mesh = geometry();
    mesh->allocate(static_cast<int>(vertices.size()));
    auto *output = mesh->vertexDataAsPoint2D();
    for (size_t i = 0; i < vertices.size(); ++i)
        output[i].set(vertices[i].x(), vertices[i].y());
    mesh->markVertexDataDirty();
    markDirty(DirtyGeometry);
}

void WireframeNode::setColor(QColor color)
{
    auto *fill = static_cast<QSGFlatColorMaterial *>(material());
    if (fill->color() == color) return;
    fill->setColor(color);
    markDirty(DirtyMaterial);
}
