#ifndef TEXTDEPTH_H
#define TEXTDEPTH_H

#include <QQuickPaintedItem>
#include <QBasicTimer>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QPainter>
#include <QImage>
#include <vector>
#include <PhotoshopWriter.h>
#include <qtToPhotoshopAPI.h>
#include <drawable.h>

#include <PhotoshopAPI.h>
#include <vector>
#include <unordered_map>

class TextDepth : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)


    QPointF closestPointOnLineSegment(
        const QPointF &segmentStart,
        const QPointF &segmentEnd,
        const QPointF &queryPoint);

    void sendLayersChangeSignal();

public:
    explicit TextDepth(qreal width, qreal height, QObject *parent = nullptr);
    void mousePressEvent(QMouseEvent *event)
    {
        QPointF localPos = event->position();        // item-local coords
        QPointF scenePos = event->scenePosition();   // scene coords
        QPointF globalPos = event->globalPosition(); // screen coords
    }

    struct ScanEdge
    {
        double x;    // current x intersection with scanline
        double dxdy; // slope inverse: Δx / Δy
        int yMin;    // first scanline this edge is active
        int yMax;    // scanline after which this edge is removed
    };

    struct Interval
    {
        double start;
        double end;
    };

    struct IPolygon
    {
        ~IPolygon() {};
        virtual std::vector<QPointF> getPoints() const = 0;
        virtual std::vector<std::pair<QPointF, QPointF>> getEdges() const {};
    };

    struct Polygon : IPolygon
    {
        std::vector<QPointF> m_points;

        std::vector<QPointF> getPoints() const override
        {
            qDebug() << "Running an overriden getPoints() function!";
            return m_points;
        };
        std::vector<std::pair<QPointF, QPointF>> getEdges() const override
        {
            std::vector<std::pair<QPointF, QPointF>> edges;
            for (int i = 0; i < m_points.size(); i++)
            {
                edges.push_back(std::make_pair(m_points[i], m_points[(i + 1) % m_points.size()]));
            }

            return edges;
        };
    };

    struct Quad : IPolygon
    {
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
                std::make_pair(back1, front1)};
        }
    };

     std::vector<ScanEdge> buildEdgeTable(const TextDepth::Quad &quad);
    void remove_me_loadPsdAndPublish() {
        using namespace NAMESPACE_PSAPI;

        // In this case we already know the bit depth but otherwise one could use the PhotoshopFile.m_Header.m_Depth
        // variable on the PhotoshopFile to figure it out programmatically. This would need to be done using the
        // "extended" read signature shown in the ExtendedSignature example.
        std::cout << "about to read" << std::endl;
        LayeredFile<bpp8_t> layeredFile = LayeredFile<bpp8_t>::read("/Users/albanypatriawan/Desktop/GitRepos/PhotoshopAPI/build/PhotoshopExamples/ReplaceImageData/ImageData.psd");
        auto targetGroupLayer = find_layer_as<bpp8_t, GroupLayer>("targets", layeredFile);

        auto &targetLayers = targetGroupLayer->layers();

        PsApiData final;
        for (auto &targetLayer : targetLayers)
        {
            std::cout << "layer has clipping mask: " << targetLayer->clipping_mask() << " for " << targetLayer->name() << std::endl;

            try {
                auto imageLayerPtr = std::dynamic_pointer_cast<ImageLayer<bpp8_t>>(targetLayer);
                if (imageLayerPtr->m_vectorMask) {
                auto vecMask = imageLayerPtr->m_vectorMask.get();

                PsApiTextData textData;
                textData.front.vectorMaskData = *vecMask;

                for (const auto& subPath : vecMask->m_subPaths) {
                    for (const auto& point : subPath.m_points) {
                    qDebug() << "Preceding (" << point.m_preceding.x << "," << point.m_preceding.y << ")"
                         << "Anchor (" << point.m_anchor.x << "," << point.m_anchor.y << ")"
                         << "Leaving (" << point.m_leaving.x << "," << point.m_leaving.y << ")";
                    if (point.m_preceding.x == point.m_anchor.x &&
                           point.m_preceding.y == point.m_anchor.y &&
                           point.m_leaving.x == point.m_anchor.x &&
                            point.m_leaving.y == point.m_anchor.y) {

                        }
                    }
                }


                final.push_back(textData);

                }
            } catch(const std::bad_cast& e) {
                qDebug() << "Failed to cast: " + std::string(e.what());
            }

        }
        qDebug() << "---------------------DONE LOADING!!!!!!!!!!!!!!!!!!!!!!!---------------------";

        m_qtData = psApiToQt(final, m_width, m_height);
        auto & firstText = m_qtData[2];
        QPainterPath & frontPath = firstText.front.vectorMaskData;
        QPainterPath backPath = frontPath;
        QTransform trans;

        QPointF centerOfFront = frontPath.boundingRect().center();
        trans.translate(centerOfFront.x(), centerOfFront.y());
        trans.scale(0.8, 0.8);
        trans.translate(-centerOfFront.x(), -centerOfFront.y());
        trans.translate(0, 130);
        backPath = backPath * trans;

        QImage baseFront(m_width, m_height, QImage::Format_ARGB32);
        baseFront.fill(QColor(255, 172, 0));

        firstText.front.baseLayer = baseFront;

        // shadows
        QColor hi(133, 67, 14);
        QColor lo(77, 24, 0);

        auto  quads = createFrontAndBackConnection(frontPath, backPath);
        QImage  base = createBackLayerBase(quads, hi);
        QImage  shadows = createBackLayerShadows(quads, lo, hi);

        firstText.back.baseLayer = base;
        firstText.back.clippedLayers.push_back(shadows);

        publishQtData();

        //	auto channels = targetLayer->get_image_data();

        //	for (auto &[key, value] : channels)
        //	{
        //		std::for_each(std::execution::par, value.begin(), value.end(), [&](uint8_t &pixelValue)
        //					  { std::cout << ".. key: " << key << ", value: " << value << std::endl; });
        //	}
    }



    // void paint(QPainter *painter) override;

    // TODO: Get rid of me
    QString text() const { return "test";  }
    void setText(const QString &text);

public slots:
    void addLayer(const QString name);
    void publishQtData();

    // TODO: Refactor me! Im so ugly!
    Q_INVOKABLE void writeToPhotoshop();

signals:
    void textChanged(); // probs dont need dis
    void layersChanged(std::vector<TextDrawable> texts);
    void notifyNewQtData(const QtData &qData);

private:
    int m_width;
    int m_height;
    QtData m_qtData;

    struct TextLayerData
    {
        // Base data
        qreal x;
        qreal y;

        qreal backRelX;
        qreal backRelY;
        qreal backSize;

        // Visuals
        std::vector<QImage> frontClippingMasks;
        QPainterPath frontPath;

        QColor m_coreShadowHiColor;
        QColor m_coreShadowLoColor;
        QColor m_atmosphereColor;

        TextLayerData()
        {
            m_coreShadowHiColor = QColor(20, 58, 100);
            m_atmosphereColor = QColor(0, 0, 0);
            m_coreShadowLoColor = QColor(10, 10, 50);
        }
    };


    void updateTextPath(TextLayerData &data);

    std::vector<Interval> insertInterval(std::vector<Interval> intervals, Interval newInterval);
    std::unordered_map<int, std::vector<TextDepth::Interval>> getScanlineIntervals(std::shared_ptr<IPolygon> polygon, int minY, int maxY, int numScanlines);

    bool intersectScanline(double scanY, const QPointF &p1, const QPointF &p2, double &outX);

    std::vector<Quad> getVisibleQuadsOptimized(const std::vector<Quad> &quads, const std::shared_ptr<IPolygon> frontPolygon);
    void updateTextPath();
    std::vector<Quad> createFrontAndBackConnection(QPainterPath front, QPainterPath back);
    QPointF deCasteljau(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3, qreal t);
    void renderQuads(const std::vector<Quad> quads, TextLayerData &data);

    // Helper methods for 3D depth effect

    void printIntervalsState(std::unordered_map<int, std::vector<TextDepth::Interval>> intervalState);

    // Returns an non-bezier approximation of a beizier-point-containing QPainterPath
    std::vector<std::vector<QPointF>> getNonBezierPath(const QPainterPath &path, int samplesPerCurve);

    // Returns a subpath-organized representation of a QPatherPath
    std::vector<std::vector<QPainterPath::Element>> getOrganizedPath(const QPainterPath &inputPath);

    const void setRasterQuadData(Quad &quad, QColor &color);
    // QColor calculateColorFromAngle(const QPointF& p1, const QPointF& p2); // DEPRECATED
    int calculateCoreShadowLoOpacityFromAngle(const QPointF &p1, const QPointF &p2);

    // Vanishing point calculation
    QPointF calculateVanishingPoint(const std::vector<std::vector<QPointF>> &frontSubpaths,
                                    const std::vector<std::vector<QPointF>> &backSubpaths);
    QPointF lineIntersection(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4);

    // QString m_text;
    // qreal m_textSize = 450;
    // qreal m_textX;
    // qreal m_textY;

    std::vector<TextLayerData> m_layers;

    // QPainterPath m_textPath;
    // QImage m_rasterCoreShadowHi;
    // QImage m_rasterCoreShadowLo;
    // QImage m_rasterAtmosphere;
    // QColor m_coreShadowHiColor;
    // QColor m_coreShadowLoColor;
    // QColor m_atmosphereColor;

    bool m_invertCoreShadow = true;

    //inline void resetRasterLayers(TextLayerData &layer, qreal width, qreal height)
    //{
    //}

    QImage createBackLayerBase(const std::vector<Quad> quads, const QColor & color);
    QImage createBackLayerShadows(const std::vector<Quad> quads, const QColor & darkestColor, const QColor & lightestColor);

    //QPointF m_vanishingPoint;

    //QPointF tmp1;
    //QPointF tmp2;
    //QPointF tmp3;
    //QPointF tmp4;

    float m_zoom = 0.7;
};

#endif // TEXTDEPTH_H
