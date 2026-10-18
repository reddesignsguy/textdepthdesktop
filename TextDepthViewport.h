#ifndef TEXTDEPTHVIEWPORT_H
#define TEXTDEPTHVIEWPORT_H
#include "rendering/ViewportScene.h"
#include <QQuickItem>
#include <QTimer>
#include <QElapsedTimer>

class TextDepthViewport : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor wireframeColor READ wireframeColor WRITE setWireframeColor NOTIFY wireframeColorChanged)
    Q_PROPERTY(qreal wireframeWidth READ wireframeWidth WRITE setWireframeWidth NOTIFY wireframeWidthChanged)
public:
    explicit TextDepthViewport(QQuickItem *parent = nullptr);
    QColor wireframeColor() const { return m_wireframeColor; }
    void setWireframeColor(QColor color);
    qreal wireframeWidth() const { return m_wireframeWidth; }
    void setWireframeWidth(qreal width);
public slots:
    void handleNewQtData(const RenderedTextDepthUnits &units);
    void handleBackPreview(const BackPreview &preview);
signals:
    void wireframeColorChanged();
    void wireframeWidthChanged();
protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
private:
    std::vector<UnitImages> m_images;
    BackPreview m_preview;
    quint64 m_documentRevision = 1;
    quint64 m_previewRevision = 1;
    QColor m_wireframeColor{0, 255, 80};
    qreal m_wireframeWidth = 1.5;
    QTimer m_pulseTimer;
    QElapsedTimer m_pulseClock;
};
#endif
