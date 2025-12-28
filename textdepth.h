#ifndef TEXTDEPTH_H
#define TEXTDEPTH_H

#include <QQuickPaintedItem>
#include <QBasicTimer>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QPainter>
#include <QImage>
#include <vector>

class TextDepth : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    struct Quad {
        QPointF front1;
        QPointF front2;
        QPointF back1;
        QPointF back2;
    };
public:

    TextDepth(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
    
    QString text() const { return m_text; }
    void setText(const QString &text);

signals:
    void textChanged();

private:
    void updateTextPath();
    void createRasterData();
    QPointF deCasteljau(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, qreal t);
    
    // Helper methods for 3D depth effect

    std::vector<std::vector<QPointF>> extractPathPoints(const QPainterPath& path, int samplesPerCurve);
    void fillQuadInRaster(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QColor& color);
    void fillPolygonScanline(const std::vector<QPointF>& points, const QColor& color);
    QColor calculateColorFromAngle(const QPointF& p1, const QPointF& p2);
    
    // Vanishing point calculation
    QPointF calculateVanishingPoint(const std::vector<std::vector<QPointF>>& frontSubpaths, 
                                     const std::vector<std::vector<QPointF>>& backSubpaths);
    QPointF lineIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4);
    
    QString m_text;
    QPainterPath m_textPath;
    QImage m_rasterImage;
    QPointF m_vanishingPoint;

    QPointF tmp1;
    QPointF tmp2;
    QPointF tmp3;
    QPointF tmp4;

};

#endif // TEXTDEPTH_H
