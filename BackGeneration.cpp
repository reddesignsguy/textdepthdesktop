#include "BackGeneration.h"
#include <QLineF>
#include <QVector2D>
#include <QtMath>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
struct Quad {
    QPointF front1, front2, back1, back2;
    std::vector<QPointF> getPoints() const { return {front1, front2, back2, back1}; }
    std::vector<std::pair<QPointF, QPointF>> getEdges() const {
        return {{front1, front2}, {front2, back2}, {back2, back1}, {back1, front1}};
    }
};
QPointF deCasteljau(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3, qreal t)
{
    // De Casteljau's algorithm for cubic Bezier curves
    // First level of interpolation
    QPointF p01 = p0 + t * (p1 - p0);
    QPointF p12 = p1 + t * (p2 - p1);
    QPointF p23 = p2 + t * (p3 - p2);

    // Second level of interpolation
    QPointF p012 = p01 + t * (p12 - p01);
    QPointF p123 = p12 + t * (p23 - p12);

    // Final interpolation - point on the curve
    QPointF result = p012 + t * (p123 - p012);

    return result;
}

std::vector<std::vector<QPointF>> getNonBezierPath(const QPainterPath &tmpPath, int samplesPerCurve)
{
    QPainterPath path = tmpPath;
    std::vector<std::vector<QPointF>> res;
    qDebug() << "num path points: " << path.elementCount();
    if (path.isEmpty())
    {
        return res;
    }

    QPointF currentPoint;
    QPointF p0, p1, p2, p3;

    std::vector<QPointF> subpath;
    for (int i = 0; i < path.elementCount(); ++i)
    {
        QPainterPath::Element element = path.elementAt(i);
        QPointF point(element.x, element.y);

        switch (element.type)
        {
        case QPainterPath::MoveToElement:
            currentPoint = point;
            p0 = point;
            subpath.push_back(point);
            break;
        case QPainterPath::LineToElement:
            currentPoint = point;
            p0 = point;
            subpath.push_back(point);
            break;

        case QPainterPath::CurveToElement:
            // This is control point 1
            p1 = point;
            break;

        case QPainterPath::CurveToDataElement:
            // Check if this is control point 2 or end point
            if (i + 1 < path.elementCount() &&
                path.elementAt(i + 1).type == QPainterPath::CurveToDataElement)
            {
                // This is control point 2
                p2 = point;
            }
            else
            {
                // This is the end point - we have all 4 points for cubic Bezier
                p3 = point;

                // Sample points along the bezier curve using deCasteljau
                // Don't include t=0 since that's already the last point added
                for (int sample = 1; sample <= samplesPerCurve; ++sample)
                {
                    qreal t = static_cast<qreal>(sample) / samplesPerCurve;
                    QPointF sampledPoint = deCasteljau(p0, p1, p2, p3, t);
                    subpath.push_back(sampledPoint);
                }

                // Update current point and p0 for next curve
                currentPoint = p3;
                p0 = p3;
            }
            break;
        }

        // If the next element is a new subpath, then clear out this subpath's buffer
        if (i + 1 >= path.elementCount() || path.elementAt(i + 1).type == QPainterPath::MoveToElement)
        {
            res.push_back(subpath);
            subpath.clear();
        }
    }

    return res;
}

int calculateCoreShadowLoOpacityFromAngle(const QPointF &p1, const QPointF &p2)
{
    // Calculate the angle between two consecutive points
    qreal dx = p2.x() - p1.x();
    qreal dy = p2.y() - p1.y();
    qreal angle = std::atan2(dy, dx);
    qreal normalizedAngle = std::abs(angle);
    if (normalizedAngle > M_PI / 2.0)
    {
        normalizedAngle = M_PI - normalizedAngle;
    }
    qreal horizontalness = 1.0 - (normalizedAngle / (M_PI / 2.0));

    int opacity = static_cast<int>(255 * (1.0 - horizontalness));
    return opacity;
}

QPointF calculateVanishingPoint(const std::vector<std::vector<QPointF>> &frontSubpaths,
                                           const std::vector<std::vector<QPointF>> &backSubpaths)
{
    if (frontSubpaths.empty() || backSubpaths.empty())
    {
        qDebug() << "No subpaths available for vanishing point calculation";
        return QPointF(0, 0);
    }

    if (frontSubpaths.size() != backSubpaths.size())
    {
        qDebug() << "Mismatch in subpath counts";
        return QPointF(0, 0);
    }

    // Use the first subpath that has enough points
    int subpathIdx = 0;

    const auto &frontPolygon = frontSubpaths[subpathIdx];
    const auto &backPolygon = backSubpaths[subpathIdx];

    // Take two different points from the same subpath
    // Use first point and a point roughly in the middle

    const QPointF &frontP1 = frontPolygon[0];
    const QPointF &backP1 = backPolygon[0];
    QLineF line1 = QLineF(frontP1, backP1);

    QLineF line2;

    if (frontSubpaths.size() == 1)
    {
        qreal greatestAngleDifference = -1;
        // Find line with. the greatest angle difference with line1 so we can get the most accurate vanishing point
        for (int i = 0; i < frontPolygon.size(); i++)
        {
            const QPointF &frontP2 = frontPolygon[i];
            const QPointF &backP2 = backPolygon[i];
            QLineF curLine = QLineF(frontP2, backP2);

            qreal angle = line1.angleTo(curLine);
            if (angle > 180.0)
                angle -= 360.0;

            if (greatestAngleDifference == -1 || angle > greatestAngleDifference)
            {
                line2 = curLine;
                greatestAngleDifference = angle;
            }
        }
    }
    // if we have more than 1 letter, then we can use any line intersection as long as we get the lines from 2 different letters as. the letters allow the liens to be as far apart as possible
    else
    {
        // assumes the front and back subpaths lengths are the same
        const auto &frontPolygon2 = frontSubpaths[frontSubpaths.size() - 1];
        const auto &backPolygon2 = backSubpaths[backSubpaths.size() - 1];

        const QPointF &frontP2 = frontPolygon2[0];
        const QPointF &backP2 = backPolygon2[0];
        line2 = QLineF(frontP2, backP2);
    }

    // temp //////////////////
    const qreal L = 10000; // big number
    QVector2D dir(line1.p2() - line1.p1());
    dir.normalize();
    //tmp1 = line1.p1() - dir.toPointF() * L;
    //tmp2 = line1.p1() + dir.toPointF() * L;

    //QVector2D dir2(line2.p2() - line2.p1());
    //dir2.normalize();
    //tmp3 = line2.p1() - dir2.toPointF() * L;
    //tmp4 = line2.p1() + dir2.toPointF() * L;
    ////////////////////

    for (int i = 0; i < frontPolygon.size(); i++)
    {
        QPointF vanishingPoint;
        QLineF::IntersectType type = line1.intersects(line2, &vanishingPoint);

        if (type == QLineF::NoIntersection)
        {
            qDebug() << "This line did not create vanishing point" << vanishingPoint;
        }
        else
        {
            qDebug() << "Calculated vanishing point:" << vanishingPoint;
            qDebug() << "===================================";
            return vanishingPoint;
        }
    }

    qDebug() << "No line intersections found:";

    return QPointF(0, 0);
}

std::vector<Quad> createFrontAndBackConnection(QPainterPath front, QPainterPath back)
{
    int num_subdivisions = 25;
    auto tmp_frontSubpaths = getNonBezierPath(front, num_subdivisions);
    auto tmp_backSubpaths = getNonBezierPath(back, num_subdivisions);
    qDebug() << "tmp_frontSubpaths size: " << tmp_frontSubpaths.size();
    // Calculate the vanishing point using the leftmost and rightmost subpaths
    QPointF vanishingPoint = calculateVanishingPoint(tmp_frontSubpaths, tmp_backSubpaths);

    std::vector<Quad> quads;
    // Abstract the points into quads which is needed for when we sort them
    for (int subpathIdx = 0; subpathIdx < tmp_frontSubpaths.size(); subpathIdx++)
    {
        qDebug() << "tmp_frontSubpaths[" << subpathIdx << "] size: " << tmp_frontSubpaths[subpathIdx].size();
        std::vector<QPointF> &frontPoints = tmp_frontSubpaths[subpathIdx];
        std::vector<QPointF> &backPoints = tmp_backSubpaths[subpathIdx];

        std::vector<Quad> quadsForThisLetter;
        for (int i = 0; i < frontPoints.size() - 1; i++)
        {
            Quad q;
            q.front1 = frontPoints[i];
            q.front2 = frontPoints[i + 1];
            q.back1 = backPoints[i];
            q.back2 = backPoints[i + 1];
            quadsForThisLetter.push_back(q);
        }

        qDebug() << "apatriawan num quads: " << quadsForThisLetter.size() << " for subpathIdx: " << subpathIdx;

        std::copy(quadsForThisLetter.begin(), quadsForThisLetter.end(), std::back_inserter(quads));
    }

    std::vector<std::pair<int, int>> subpathQuadsDistToVanishingPointIndices;
    int i = 0;

    // Calculate dist to vanishing points for each quad
    for (const auto &quad : quads)
    {
        QPointF avg(0, 0);
        avg += quad.front1;
        avg += quad.front2;
        avg += quad.back1;
        avg += quad.back2;
        avg /= 4;

        subpathQuadsDistToVanishingPointIndices.push_back(std::pair<int, int>{i,
                                                                              QLineF(avg, vanishingPoint).length()});
        i++;
    }

    // Sort the indices
    std::sort(subpathQuadsDistToVanishingPointIndices.begin(), subpathQuadsDistToVanishingPointIndices.end(),
              [](const auto a, const auto b)
              {
                  int a_distToVanishingPoint = a.second;
                  int b_distToVanishingPoint = b.second;
                  return a_distToVanishingPoint > b_distToVanishingPoint;
              });

    // Apply the sorted indices to sorting the quads
    std::vector<Quad> sortedQuads;
    for (const auto [index, _] : subpathQuadsDistToVanishingPointIndices)
    {
        Quad quad = quads[index];
        sortedQuads.push_back(quad);
    }

    quads = sortedQuads;

    return quads;
}

QImage createBackLayerBase(const std::vector<Quad> quads, int width, int height, const QColor & color)
{
    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull()) throw std::runtime_error("Could not allocate back layers.");
    image.fill(Qt::transparent);
    for (int i = 0; i < quads.size(); i++)
    {
        auto quad = quads[i];

        const QPointF &frontP1 = quad.front1;
        const QPointF &frontP2 = quad.front2;

        const auto &points = quad.getPoints();
        qreal minY = points[0].y();
        qreal maxY = points[0].y();

        for (const QPointF &p : points)
        {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        int startY = qMax(0, static_cast<int>(qFloor(minY)));
        int endY = qMin(height - 1, static_cast<int>(qCeil(maxY)));

        // DRAWING ALGORITHM
        // For each "scanline", a horizontal line that goes down the screen every 1 pixel,
        // This loop scales in time complexity based on the text height
        for (int y = startY; y <= endY; ++y)
        {
            std::vector<qreal> intersections;

            for (auto edge : quad.getEdges())
            {
                const QPointF &p1 = edge.first;
                const QPointF &p2 = edge.second;

                if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y))
                {
                    // Calculate x intersection
                    qreal t = (y - p1.y()) / (p2.y() - p1.y());
                    qreal x = p1.x() + t * (p2.x() - p1.x());
                    intersections.push_back(x);
                }
            }

            // Sort intersections
            std::sort(intersections.begin(), intersections.end());

            // Fill between pairs of intersections
            for (int i = 0; i + 1 < intersections.size(); i += 2)
            {
                double intersect1 = intersections[i];
                double intersect2 = intersections[i + 1];
                int startX = qMax(0, static_cast<int>(qFloor(intersect1)));
                int endX = qMin(width - 1, static_cast<int>(qFloor(intersect2)));

                for (int x = startX; x <= endX; ++x)
                {
                    image.setPixelColor(x, y, color);
                }
            }
        }
    }
    return image;
}

QImage createBackLayerShadows(const std::vector<Quad> quads, int width, int height, const QColor & darkestColor, const QColor & lightestColor)
{
    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull()) throw std::runtime_error("Could not allocate back layers.");
    image.fill(Qt::transparent);
    for (int i = 0; i < quads.size(); i++)
    {
        auto quad = quads[i];

        const QPointF &frontP1 = quad.front1;
        const QPointF &frontP2 = quad.front2;

        // =========== THIS IS WHAT MAKES THIS FUNCTION SPECIAL ==============
        // Alpha value calculations for the core shadow "lo" layer
        int interpolatedCoreShadowLoOpacity = calculateCoreShadowLoOpacityFromAngle(frontP1, frontP2);
        interpolatedCoreShadowLoOpacity = 255 - interpolatedCoreShadowLoOpacity;
        QColor interpolatedCoreShadowLoColor = darkestColor;
        interpolatedCoreShadowLoColor.setAlpha(interpolatedCoreShadowLoOpacity);
        // ===================================================================

        const auto &points = quad.getPoints();
        qreal minY = points[0].y();
        qreal maxY = points[0].y();

        for (const QPointF &p : points)
        {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        int startY = qMax(0, static_cast<int>(qFloor(minY)));
        int endY = qMin(height - 1, static_cast<int>(qCeil(maxY)));

        // DRAWING ALGORITHM
        // For each "scanline", a horizontal line that goes down the screen every 1 pixel,
        // This loop scales in time complexity based on the text height
        for (int y = startY; y <= endY; ++y)
        {
            std::vector<qreal> intersections;

            for (auto edge : quad.getEdges())
            {
                const QPointF &p1 = edge.first;
                const QPointF &p2 = edge.second;

                if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y))
                {
                    // Calculate x intersection
                    qreal t = (y - p1.y()) / (p2.y() - p1.y());
                    qreal x = p1.x() + t * (p2.x() - p1.x());
                    intersections.push_back(x);
                }
            }

            // Sort intersections
            std::sort(intersections.begin(), intersections.end());

            // Fill between pairs of intersections
            for (int i = 0; i + 1 < intersections.size(); i += 2)
            {
                double intersect1 = intersections[i];
                double intersect2 = intersections[i + 1];
                int startX = qMax(0, static_cast<int>(qFloor(intersect1)));
                int endX = qMin(width - 1, static_cast<int>(qFloor(intersect2)));

                for (int x = startX; x <= endX; ++x)
                {
                    image.setPixelColor(x, y, interpolatedCoreShadowLoColor); // Tmp
                }
            }
        }
    }
    return image;
}
} // namespace

Back generateBack(const Front &front, QSize canvasSize,
                  QPointF relativePosition, QPointF relativeSize)
{
    const auto &path = front.vectorMaskData;
    const auto bounds = path.boundingRect();
    if (path.isEmpty() || bounds.width() <= 0 || bounds.height() <= 0)
        throw std::invalid_argument("A non-empty front vector mask is required.");
    const auto center = bounds.center();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.scale(relativeSize.x(), relativeSize.y());
    transform.translate(-center.x(), -center.y());
    transform.translate(relativePosition.x(), relativePosition.y());
    const auto quads = createFrontAndBackConnection(path, path * transform);
    Back back;
    back.baseLayer = createBackLayerBase(quads, canvasSize.width(), canvasSize.height(), QColor(133, 67, 14));
    back.clippedLayers.push_back(createBackLayerShadows(quads, canvasSize.width(), canvasSize.height(),
                                                       QColor(77, 24, 0), QColor(133, 67, 14)));
    return back;
}
