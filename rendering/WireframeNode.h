#ifndef WIREFRAMENODE_H
#define WIREFRAMENODE_H
#include <QSGGeometryNode>
#include <QPainterPath>
#include <QColor>

// GPU stroke geometry. Rebuild only when the path/width changes; color is cheap.
class WireframeNode final : public QSGGeometryNode
{
public:
    WireframeNode();
    void setPath(const QPainterPath &path, qreal width);
    void setColor(QColor color);
};
#endif
