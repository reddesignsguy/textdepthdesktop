#ifndef TEXTDEPTHVIEWPORT_H
#define TEXTDEPTHVIEWPORT_H
#include "TextDepthUnit.h"
#include "BackPreview.h"
#include "drawable.h"
#include <QQuickPaintedItem>
#include <QTimer>
#include <QElapsedTimer>

class TextDepthViewport : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor wireframeColor READ wireframeColor WRITE setWireframeColor NOTIFY wireframeColorChanged)
public:
    explicit TextDepthViewport(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    QColor wireframeColor() const { return m_wireframeColor; }
    void setWireframeColor(QColor color);
public slots:
    void drawLayers(std::vector<TextDrawable> texts);
    void handleNewQtData(const RenderedTextDepthUnits &units);
    void handleBackPreview(const BackPreview &preview);
signals:
    void wireframeColorChanged();
private:
    struct RenderItem { QUuid id; TextDrawable drawable; };
    std::vector<RenderItem> m_items;
    // Only the current transient preview survives until the next paint/event.
    BackPreview m_preview;
    QColor m_wireframeColor{0, 255, 80};
    QTimer m_pulseTimer;
    QElapsedTimer m_pulseClock;
};
#endif
