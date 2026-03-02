#ifndef TEXTDEPTH_H
#define TEXTDEPTH_H

#include <QQuickPaintedItem>
#include <QBasicTimer>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QPainter>
#include <QImage>
#include <vector>
#include <climits>

class TextDepth : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

    struct ScanEdge {
        double x;        // current x intersection with scanline
        double dxdy;     // slope inverse: Δx / Δy
        int yMin;        // first scanline this edge is active
        int yMax;        // scanline after which this edge is removed
    };


    struct Interval {
        double start;
        double end;
    };

    struct IPolygon
    {
        ~IPolygon() {};
        virtual std::vector<QPointF> getPoints() const{qDebug() << "IMPLEMENT ME!";};
        virtual std::vector<std::pair<QPointF, QPointF>> getEdges() const{};
    };

    struct Polygon : IPolygon
    {
        std::vector<QPointF> points;


        std::vector<QPointF> getPoints() const override { qDebug() << "Running an overriden getPoints() function!"; return points; };
        std::vector<std::pair<QPointF, QPointF>> getEdges() const override
        {
            std::vector<std::pair<QPointF, QPointF>> edges;
            for (int i = 0; i < points.size(); i ++)
            {
                edges.push_back(std::make_pair(points[i], points[  (i + 1) % points.size()  ]));
            }

            return edges;
        };

    };

    struct Quad : IPolygon {
        QPointF front1;
        QPointF front2;
        QPointF back1;
        QPointF back2;

        std::vector<QPointF> getPoints() const override
        {
            return std::vector<QPointF>{front1, front2, back2, back1};
        };

        std::vector<std::pair<QPointF, QPointF>> getEdges() const override
        {
            return {
                std::make_pair(front1, front2),
                std::make_pair(front2, back2),
                std::make_pair(back2, back1),
                std::make_pair(back1, front1)
            };
        }
    };

    std::vector<ScanEdge> buildEdgeTable(const Quad& quad);

    QPointF closestPointOnLineSegment(
        const QPointF& segmentStart,
        const QPointF& segmentEnd,
        const QPointF& queryPoint);
public:

    TextDepth(QQuickItem *parent = nullptr);

    void mousePressEvent(QMouseEvent* event)
    {
        QPointF localPos = event->position();        // item-local coords
        QPointF scenePos = event->scenePosition();   // scene coords
        QPointF globalPos = event->globalPosition(); // screen coords

        qDebug() << "scene pos: "<< scenePos;

        qDebug() << "local pos (real!): " << localPos;
        qDebug() << "global pos: " << globalPos;
        qDebug() << "-----------" ;
    }

    void paint(QPainter *painter) override;
    
    QString text() const { return m_text; }
    void setText(const QString &text);

signals:
    void textChanged();

private:
    std::vector<Interval> insertInterval(std::vector<Interval> intervals, Interval newInterval);
    std::unordered_map<int, std::vector<TextDepth::Interval>> getScanlineIntervals(std::shared_ptr<IPolygon> polygon, int minY, int maxY, int numScanlines);

    bool intersectScanline(double scanY, const QPointF& p1, const QPointF& p2, double& outX);

    std::vector<Quad> getVisibleQuadsOptimized(const std::vector<Quad>& quads, const std::shared_ptr<IPolygon> frontPolygon);
    void updateTextPath();
    void createRasterData();
    QPointF deCasteljau(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, qreal t);
    void renderQuads(const std::vector<Quad> quads);

    // Helper methods for 3D depth effect

    void printIntervalsState(std::unordered_map<int, std::vector<TextDepth::Interval>> intervalState);
    std::vector<std::vector<QPointF>> extractPathPoints(const QPainterPath& path, int samplesPerCurve);


    const void setRasterQuadData(Quad& quad, QColor& color);
    QColor calculateColorFromAngle(const QPointF& p1, const QPointF& p2); // DEPRECATED
    int calculateCoreShadowLoOpacityFromAngle(const QPointF& p1, const QPointF& p2);
    
    // Vanishing point calculation
    QPointF calculateVanishingPoint(const std::vector<std::vector<QPointF>>& frontSubpaths, 
                                     const std::vector<std::vector<QPointF>>& backSubpaths);
    QPointF lineIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4);
    
    QString m_text;
    qreal m_textSize = 450;
    qreal m_textX;
    qreal m_textY;

    QPainterPath m_textPath;
    QImage m_rasterCoreShadowHi;
    QImage m_rasterCoreShadowLo;
    QImage m_rasterAtmosphere;
    QColor m_coreShadowHiColor;
    QColor m_coreShadowLoColor;
    QColor m_atmosphereColor;

    bool m_invertCoreShadow = true;

    inline void resetRasterLayers()
    {
            m_rasterCoreShadowHi = QImage(width(), height(), QImage::Format_ARGB32);
            m_rasterCoreShadowLo = QImage(width(), height(), QImage::Format_ARGB32);
            m_rasterAtmosphere = QImage(width(), height(), QImage::Format_ARGB32);
            m_rasterCoreShadowHi.fill(Qt::transparent);
            m_rasterCoreShadowLo.fill(Qt::transparent);
            m_rasterAtmosphere.fill(Qt::transparent);
    }

    QPointF m_vanishingPoint;

    QPointF tmp1;
    QPointF tmp2;
    QPointF tmp3;
    QPointF tmp4;


    float m_zoom = 0.7;

};

#endif // TEXTDEPTH_H
