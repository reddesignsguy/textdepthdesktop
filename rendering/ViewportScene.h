#ifndef VIEWPORTSCENE_H
#define VIEWPORTSCENE_H
#include "TextDepthUnit.h"
#include "BackPreview.h"
#include <QSGNode>
#include <QColor>
#include <vector>

class QQuickWindow;
class QSGOpacityNode;
class WireframeNode;

// CPU preparation happens only on document changes. Images are implicitly shared.
struct UnitImages {
    QUuid id;
    QImage front;
    QImage back;
};
std::vector<UnitImages> prepareUnitImages(const RenderedTextDepthUnits &units);

// Owned by Qt on the render thread. Each unit has back, preview, front children
// in that order, so ordinary scene-graph ordering handles overlap correctly.
class ViewportScene final : public QSGNode
{
public:
    void setUnits(QQuickWindow *window, const std::vector<UnitImages> &units);
    void setPreview(const BackPreview &preview, qreal width);
    void setPreviewColor(QColor color);
    quint64 documentRevision = 0;
    quint64 previewRevision = 0;
private:
    struct UnitNodes {
        QUuid id;
        QSGOpacityNode *back;
        QSGOpacityNode *preview;
        WireframeNode *wireframe;
    };
    std::vector<UnitNodes> m_units;
};
#endif
