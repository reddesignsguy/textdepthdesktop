#include "textdepth.h"
#include <QFont>
#include <QFontMetrics>
#include <QDebug>
#include <cmath>
#include <limits>

TextDepth::TextDepth(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_text("TextDepth")
{
    // Set default size
    setWidth(400);
    setHeight(300);
    
    // Create initial text path and raster data
    updateTextPath();
    createRasterData();
}

void TextDepth::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        updateTextPath();
        createRasterData();
        emit textChanged();
        update(); // Trigger repaint
    }
}

void TextDepth::updateTextPath()
{
    m_textPath = QPainterPath();
    
    if (m_text.isEmpty()) {
        return;
    }
    
    QFont font;
    font.setPixelSize(80);
    font.setBold(true);
    
    // Calculate text position to center it
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(m_text);
    
    qreal x = (width() - textRect.width()) / 2.0 - textRect.x();
    qreal y = (height() + textRect.height()) / 2.0 - textRect.y();
    
    m_textPath.addText(x, y, font, m_text);
    
    // Set fill rule to WindingFill to properly fill holes in letters like 'e', 'o', 'a'
    m_textPath.setFillRule(Qt::WindingFill);
}

void TextDepth::createRasterData()
{
    // Create a raster image matching the canvas size
    int imageWidth = static_cast<int>(width());
    int imageHeight = static_cast<int>(height());
    
    if (imageWidth <= 0 || imageHeight <= 0) {
        imageWidth = 400;
        imageHeight = 300;
    }
    
    m_rasterImage = QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
    m_rasterImage.fill(Qt::transparent);
    
    if (m_textPath.isEmpty()) {
        return;
    }
    
    // Create the smaller back text path (same as in paint())
    QFont smallerFont;
    smallerFont.setPixelSize(64); // 80% of 80
    smallerFont.setBold(true);
    
    QFontMetrics smallerMetrics(smallerFont);
    QRect smallerTextRect = smallerMetrics.boundingRect(m_text);
    
    qreal smallerX = (width() - smallerTextRect.width()) / 2.0 - smallerTextRect.x();
    qreal smallerY = (height() + smallerTextRect.height()) / 2.0 - smallerTextRect.y();
    
    QPainterPath smallerTextPath;
    smallerTextPath.addText(smallerX, smallerY, smallerFont, m_text);
    smallerTextPath.setFillRule(Qt::WindingFill);
    
    // Use toSubpathPolygons() to get individual subpaths for each letter
    // This prevents conjoining planes between different letters
    // QList<QPolygonF> frontSubpaths = m_textPath.toSubpathPolygons();
    // QList<QPolygonF> backSubpaths = smallerTextPath.toSubpathPolygons();
    auto tmp_frontSubpaths = extractPathPoints(m_textPath, 10);
    auto tmp_backSubpaths = extractPathPoints(smallerTextPath, 10);
   // Calculate the vanishing point using the leftmost and rightmost subpaths
   m_vanishingPoint = calculateVanishingPoint(tmp_frontSubpaths, tmp_backSubpaths);

    std::vector<std::vector<Quad>> quads;
    // Abstract the points into quads which is needed for when we sort them
    for (int subpathIdx = 0; subpathIdx < tmp_frontSubpaths.size(); subpathIdx ++)
    {
        std::vector<QPointF>& frontPoints = tmp_frontSubpaths[subpathIdx];
        std::vector<QPointF>& backPoints = tmp_backSubpaths[subpathIdx];

        std::vector<Quad> subpathQuads;
        for (int i = 0; i < frontPoints.size() - 1; i ++)
        {
            Quad q;
            q.front1 = frontPoints[i];
            q.front2 = frontPoints[i + 1];
            q.back1 = backPoints[i];
            q.back2 = backPoints[i+1];
            
            subpathQuads.push_back(q);
        }

        // Make a quad for the first and last pair of points
        // Quad q;
        // q.front1 = frontPoints[0];
        // q.front2 = frontPoints[frontPoints.size() - 1];
        // q.back1 = backPoints[0];
        // q.back2 = backPoints[backPoints.size() - 1];
        // subpathQuads.push_back(q);

        qDebug() << "apatriawan num quads: " <<  subpathQuads.size() << " for subpathIdx: " << subpathIdx;


        quads.push_back(subpathQuads);
    }

    // Sort subpath quads by whoever is closest to the vanishing point
    for (int subpathIdx = 0; subpathIdx < quads.size(); subpathIdx ++)
    {
        const auto& subpathQuads = quads[subpathIdx];
        std::vector<std::pair<int, int>> subpathQuadsDistToVanishingPointIndices;
        int i = 0;
        // Calculate vanishing points for each quad
        for (const auto& quad : subpathQuads)
        {
            auto pos = QLineF(quad.front1, quad.front2).center();
            subpathQuadsDistToVanishingPointIndices.push_back(std::pair<int,int>{i,
                                                        QLineF(pos, m_vanishingPoint).length()});
            i++;
        }

        // Sort the indices
        std::sort(subpathQuadsDistToVanishingPointIndices.begin(), subpathQuadsDistToVanishingPointIndices.end(),
                  [](const auto a, const auto b) {
                      int a_distToVanishingPoint = a.second;
                      int b_distToVanishingPoint = b.second;
                      return a_distToVanishingPoint > b_distToVanishingPoint;
                  });

        // Apply the sorted indices to sorting the quads
        std::vector<Quad> sortedQuads;
        for (const auto [index, _] : subpathQuadsDistToVanishingPointIndices)
        {
            Quad quad = subpathQuads[index];
            sortedQuads.push_back(quad);
        }

        quads[subpathIdx] = sortedQuads;
    }

    std::vector<std::vector<Quad>> sortedQuads;

    // Make the letters on the side the first to be rendered
    int left = 0;
    int right = quads.size() - 1;
    while (left <= right)
    {
        if (left == right)
        {
            sortedQuads.push_back(quads[left]);
            break;
        }

        sortedQuads.push_back(quads[left]);
        sortedQuads.push_back(quads[right]);
        left ++;
        right --;
    }
    quads = sortedQuads;

    
    // Debug output
    // qDebug() << "=== 3D Depth Effect (Using Subpath Polygons) ===";
    // qDebug() << "Front subpaths:" << frontSubpaths.size();
    // qDebug() << "Back subpaths:" << backSubpaths.size();
    
    // Ensure we have matching subpaths
    if (tmp_frontSubpaths.size() != tmp_backSubpaths.size()) {
        qDebug() << "Warning: Mismatch in subpath counts!";
        return;
    }
    
    // Color for the depth/side surfaces - match back text color, fully opaque
    QColor depthColor(40, 96, 160, 255); // Same as back text, fully opaque
    
    int totalQuads = 0;
    
    // Process each subpath (letter) separately
    for (int subpathIdx = 0; subpathIdx < quads.size(); ++subpathIdx) {
        const auto subpathQuad = quads[subpathIdx];
        
        qDebug() << "Processing subpath" << subpathIdx 
                 << "- Points:" << subpathQuad.size() ;
        
        // For quad, render it
        for (int i = 0; i < subpathQuad.size() + 1; i++) {
            // Get two consecutive points from front polygon
            const auto quad = subpathQuad[i];

            const QPointF& frontP1 = quad.front1;
            const QPointF& frontP2 = quad.front2;
            QColor depthColor = calculateColorFromAngle(frontP1, frontP2);
            
            // Get corresponding points from back polygon
            const QPointF& backP1 = quad.back1;
            const QPointF& backP2 = quad.back2;
            
            // Create quadrilateral: front[i] -> front[i+1] -> back[i+1] -> back[i]
            // This forms the "side" surface connecting the two text planes
            fillQuadInRaster(frontP1, frontP2, backP2, backP1, depthColor);
            totalQuads++;
        }
        
        // qDebug() << "  Created" << (minPoints - 1) << "quads for subpath" << subpathIdx;
    }

    // for (int subpathIdx = 0; subpathIdx < frontSubpaths.size(); ++subpathIdx) {
    //     const auto frontPolygon = frontSubpaths[subpathIdx];
    //     const auto backPolygon = backSubpaths[subpathIdx];
        
    //     qDebug() << "Processing subpath" << subpathIdx 
    //              << "- Front points:" << frontPolygon.size() 
    //              << "Back points:" << backPolygon.size();
        
    //     // Ensure both polygons have the same number of points
    //     int minPoints = qMin(frontPolygon.size(), backPolygon.size());

    //     if (frontPolygon.size() != backPolygon.size())
    //     {
    //         qDebug() << " Polygon does not have the same number of poiters";
    //         continue;    
    //     }
        
    //     if (minPoints < 2) {
    //         qDebug() << "  Skipping subpath" << subpathIdx << "- not enough points";
    //         continue;
    //     }
        
    //     // For each consecutive pair of points in this subpath, create a quadrilateral
    //     for (int i = 0; i + 1 < minPoints; ++i) {
    //         // Get two consecutive points from front polygon
    //         const QPointF& frontP1 = frontPolygon[i];
    //         const QPointF& frontP2 = frontPolygon[i + 1];
    //         QColor depthColor = calculateColorFromAngle(frontP1, frontP2);
            
    //         // Get corresponding points from back polygon
    //         const QPointF& backP1 = backPolygon[i];
    //         const QPointF& backP2 = backPolygon[i + 1];
            
    //         // Create quadrilateral: front[i] -> front[i+1] -> back[i+1] -> back[i]
    //         // This forms the "side" surface connecting the two text planes
    //         fillQuadInRaster(frontP1, frontP2, backP2, backP1, depthColor);
    //         totalQuads++;
    //     }
        
    //     qDebug() << "  Created" << (minPoints - 1) << "quads for subpath" << subpathIdx;
    // }
    
    qDebug() << "Total quads created:" << totalQuads;
}

QPointF TextDepth::deCasteljau(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, qreal t)
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

std::vector<std::vector<QPointF>> TextDepth::extractPathPoints(const QPainterPath& path, int samplesPerCurve)
{
    std::vector<std::vector<QPointF>> res;
    // QList<QPolygonF> subpaths = path.toSubpathPolygons();
    // QList<QPolygonF> backSubpaths = smallerTextPath.toSubpathPolygons();
    if (path.isEmpty()) {
        return res;
    }
    
    QPointF currentPoint;
    QPointF p0, p1, p2, p3;
    
    std::vector<QPointF> subpath;
    for (int i = 0; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);
        QPointF point(element.x, element.y);
        
        switch (element.type) {
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
                    path.elementAt(i + 1).type == QPainterPath::CurveToDataElement) {
                    // This is control point 2
                    p2 = point;
                } else {
                    // This is the end point - we have all 4 points for cubic Bezier
                    p3 = point;
                    
                    // Sample points along the bezier curve using deCasteljau
                    // Don't include t=0 since that's already the last point added
                    for (int sample = 1; sample <= samplesPerCurve; ++sample) {
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

void TextDepth::fillPolygonScanline(const std::vector<QPointF>& points, const QColor& color)
{
    if (points.size() < 3 || m_rasterImage.isNull()) {
        return;
    }
    
    // Find bounding box
    qreal minY = points[0].y();
    qreal maxY = points[0].y();
    
    for (const QPointF& p : points) {
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }
    
    int startY = qMax(0, static_cast<int>(qFloor(minY)));
    int endY = qMin(m_rasterImage.height() - 1, static_cast<int>(qCeil(maxY)));
    
    // For each scanline
    for (int y = startY; y <= endY; ++y) {
        std::vector<qreal> intersections;
        
        // Find intersections with polygon edges
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            
            const QPointF& p1 = points[i];
            const QPointF& p2 = points[j];
            
            // Check if edge crosses scanline
            if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y)) {
                // Calculate x intersection
                qreal t = (y - p1.y()) / (p2.y() - p1.y());
                qreal x = p1.x() + t * (p2.x() - p1.x());
                intersections.push_back(x);
            }
        }
        
        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
        
        // Fill between pairs of intersections
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int startX = qMax(0, static_cast<int>(qCeil(intersections[i])));
            int endX = qMin(m_rasterImage.width() - 1, static_cast<int>(qFloor(intersections[i + 1])));
            
            for (int x = startX; x <= endX; ++x) {
                m_rasterImage.setPixel(x, y, color.rgba());
            }
        }
    }
}

void TextDepth::fillQuadInRaster(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QColor& color)
{
    // Create a quadrilateral from 4 points and fill it
    std::vector<QPointF> quad = {p0, p1, p2, p3};
    fillPolygonScanline(quad, color);
}

QColor TextDepth::calculateColorFromAngle(const QPointF& p1, const QPointF& p2)
{
    // Calculate the angle between two consecutive points
    qreal dx = p2.x() - p1.x();
    qreal dy = p2.y() - p1.y();
    
    // Calculate angle in radians using atan2
    qreal angle = std::atan2(dy, dx);
    
    // Normalize angle to [0, π] range
    // We use absolute value since horizontal can be 0° or 180°
    qreal normalizedAngle = std::abs(angle);
    if (normalizedAngle > M_PI / 2.0) {
        normalizedAngle = M_PI - normalizedAngle;
    }
    
    // Calculate "horizontalness" factor (0 = vertical, 1 = horizontal)
    // When angle is 0 (horizontal), horizontalness = 1
    // When angle is π/2 (vertical), horizontalness = 0
    qreal horizontalness = 1.0 - (normalizedAngle / (M_PI / 2.0));
    
    // Define color range (darker to lighter blue)
    // Darker blue for horizontal: RGB(20, 48, 80)
    // Lighter blue for vertical: RGB(60, 144, 240)
    int minR = 20, minG = 48, minB = 80;
    int maxR = 60, maxG = 144, maxB = 240;
    
    // Interpolate based on horizontalness
    int r = static_cast<int>(minR + (maxR - minR) * (1.0 - horizontalness));
    int g = static_cast<int>(minG + (maxG - minG) * (1.0 - horizontalness));
    int b = static_cast<int>(minB + (maxB - minB) * (1.0 - horizontalness));
    
    return QColor(r, g, b, 255);
}

QPointF TextDepth::lineIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4)
{
    // Calculate line intersection using parametric form
    // Line 1: p1 + t * (p2 - p1)
    // Line 2: p3 + s * (p4 - p3)
    
    qreal x1 = p1.x(), y1 = p1.y();
    qreal x2 = p2.x(), y2 = p2.y();
    qreal x3 = p3.x(), y3 = p3.y();
    qreal x4 = p4.x(), y4 = p4.y();
    
    qreal denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    
    // Check if lines are parallel
    if (qAbs(denom) < 1e-10) {
        qDebug() << "Lines are parallel, no intersection";
        return QPointF(0, 0);
    }
    
    qreal t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    
    // Calculate intersection point
    qreal intersectX = x1 + t * (x2 - x1);
    qreal intersectY = y1 + t * (y2 - y1);
    
    return QPointF(intersectX, intersectY);
}

QPointF TextDepth::calculateVanishingPoint(const std::vector<std::vector<QPointF>>& frontSubpaths, 
                                            const std::vector<std::vector<QPointF>>& backSubpaths)
{
    if (frontSubpaths.empty() || backSubpaths.empty()) {
        qDebug() << "No subpaths available for vanishing point calculation";
        return QPointF(0, 0);
    }
    
    if (frontSubpaths.size() != backSubpaths.size()) {
        qDebug() << "Mismatch in subpath counts";
        return QPointF(0, 0);
    }
    
    // Use the first subpath that has enough points
    int subpathIdx = 0;
    // for (size_t i = 0; i < frontSubpaths.size(); ++i) {
    //     if (frontSubpaths[i].size() >= 2 && backSubpaths[i].size() >= 2) {
    //         subpathIdx = i;
    //         break;
    //     }
    // }
    
    // if (subpathIdx == -1) {
    //     qDebug() << "No subpath with enough points found";
    //     return QPointF(0, 0);
    // }
    
    const auto& frontPolygon = frontSubpaths[subpathIdx];
    const auto& backPolygon = backSubpaths[subpathIdx];
    
    qDebug() << "=== Vanishing Point Calculation ===";
    qDebug() << "Using subpath index:" << subpathIdx;
    qDebug() << "Front points:" << frontPolygon.size();
    qDebug() << "Back points:" << backPolygon.size();
    
    // Take two different points from the same subpath
    // Use first point and a point roughly in the middle

    const QPointF& frontP1 = frontPolygon[0];
    const QPointF& backP1 = backPolygon[0];
    QLineF line1 = QLineF(frontP1, backP1);
    
    QLineF line2;

    if (frontSubpaths.size() == 1)
    {
        qreal greatestAngleDifference = -1;
        //Find line with. the greatest angle difference with line1 so we can get the most accurate vanishing point
        for (int i = 0; i < frontPolygon.size(); i ++)
        {
            const QPointF& frontP2 = frontPolygon[i];
            const QPointF& backP2 = backPolygon[i];
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
        const auto& frontPolygon2 = frontSubpaths[frontSubpaths.size() - 1];
        const auto& backPolygon2 = backSubpaths[backSubpaths.size() - 1];

        const QPointF& frontP2 = frontPolygon2[0];
        const QPointF& backP2 = backPolygon2[0];
        line2 = QLineF(frontP2, backP2);
    }

    // temp //////////////////
    const qreal L = 10000; // big number
    QVector2D dir(line1.p2() - line1.p1());
    dir.normalize();
     tmp1 = line1.p1() - dir.toPointF() * L;
     tmp2 = line1.p1() + dir.toPointF() * L;

    QVector2D dir2(line2.p2() - line2.p1());
    dir2.normalize();
     tmp3 = line2.p1() - dir2.toPointF() * L;
     tmp4 = line2.p1() + dir2.toPointF() * L;
////////////////////

    for (int i = 0; i < frontPolygon.size(); i ++)
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

    return QPointF(0,0);

    
    // Calculate the intersection of the two perspective lines
    // Line 1: from frontP1 to backP1
    // Line 2: from frontP2 to backP2
    // QPointF vanishingPoint = lineIntersection(frontP1, backP1, frontP2, backP2);

}

void TextDepth::paint(QPainter *painter)
{
    qDebug() << "changin!" ;
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    painter->fillRect(0, 0, width(), height(), QColor(240, 240, 240));
    
    if (m_textPath.isEmpty()) {
        return;
    }
    
    // Create smaller duplicate text path (80% of original size)
    QFont smallerFont;
    smallerFont.setPixelSize(64); // 80% of 80
    smallerFont.setBold(true);
    
    QFontMetrics smallerMetrics(smallerFont);
    QRect smallerTextRect = smallerMetrics.boundingRect(m_text);
    
    qreal smallerX = (width() - smallerTextRect.width()) / 2.0 - smallerTextRect.x();
    qreal smallerY = (height() + smallerTextRect.height()) / 2.0 - smallerTextRect.y();
    
    QPainterPath smallerTextPath;
    smallerTextPath.addText(smallerX, smallerY, smallerFont, m_text);
    smallerTextPath.setFillRule(Qt::WindingFill);
    
    // LAYER 1: Draw smaller duplicate text first (bottom layer)
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 96, 160));
    painter->drawPath(smallerTextPath);
    
    // LAYER 2: Draw pure raster data (middle layer)
    // This demonstrates drawing raw pixel/bitmap data between the text layers
    if (!m_rasterImage.isNull()) {
        // Center the raster image
        qreal rasterX = (width() - m_rasterImage.width()) / 2.0;
        qreal rasterY = (height() - m_rasterImage.height()) / 2.0;
        
        // Draw the raster image directly - this is pure pixel data
        painter->drawImage(QPointF(rasterX, rasterY), m_rasterImage);
    }
    
    // LAYER 3: Draw main text on top (top layer)
    painter->setBrush(QColor(50, 120, 200));
    painter->drawPath(m_textPath);
    
    // LAYER 4: Draw vanishing point as a visible dot
    if (!m_vanishingPoint.isNull() && m_vanishingPoint.x() != 0 && m_vanishingPoint.y() != 0) {
        // Draw a bright red dot for the vanishing point
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 0, 0, 255)); // Bright red
        
        // Draw a circle with radius 8 pixels
        qreal radius = 8.0;
        painter->drawEllipse(m_vanishingPoint, radius, radius);
        
        // Draw a white outline for better visibility
        painter->setPen(QPen(QColor(255, 255, 255, 255), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(m_vanishingPoint, radius, radius);
        
        qDebug() << "Drawing vanishing point at:" << m_vanishingPoint;
    }

    painter->drawLines(QList<QLineF>{
        QLineF(tmp1, tmp2),
        QLineF(tmp3, tmp4)
    });

    
    // Print all bezier points and evaluate curves using De Casteljau's algorithm
    qDebug() << "=== Front Text Bezier Points with De Casteljau Evaluation ===";
    
    QPointF currentPoint;
    QPointF p0, p1, p2, p3;
    
    for (int i = 0; i < m_textPath.elementCount(); ++i) {
        QPainterPath::Element element = m_textPath.elementAt(i);
        QPointF point(element.x, element.y);
        
        QString elementType;
        switch (element.type) {
            case QPainterPath::MoveToElement:
                elementType = "MoveTo";
                currentPoint = point;
                p0 = point;
                // qDebug() << QString("Point %1: %2 (%3, %4)")
                //             .arg(i)
                //             .arg(elementType)
                //             .arg(element.x, 0, 'f', 2)
                //             .arg(element.y, 0, 'f', 2);
                break;
                
            case QPainterPath::LineToElement:
                elementType = "LineTo";
                currentPoint = point;
                p0 = point;
                // qDebug() << QString("Point %1: %2 (%3, %4)")
                //             .arg(i)
                //             .arg(elementType)
                //             .arg(element.x, 0, 'f', 2)
                //             .arg(element.y, 0, 'f', 2);
                break;
                
            case QPainterPath::CurveToElement:
                elementType = "CurveTo";
                p1 = point;
                // qDebug() << QString("Point %1: %2 (%3, %4) - Control Point 1")
                //             .arg(i)
                //             .arg(elementType)
                //             .arg(element.x, 0, 'f', 2)
                //             .arg(element.y, 0, 'f', 2);
                break;
                
            case QPainterPath::CurveToDataElement:
                elementType = "CurveToData";
                // Check if this is the second control point or the end point
                if (i + 1 < m_textPath.elementCount() && 
                    m_textPath.elementAt(i + 1).type == QPainterPath::CurveToDataElement) {
                    // This is control point 2
                    p2 = point;
                    // qDebug() << QString("Point %1: %2 (%3, %4) - Control Point 2")
                    //             .arg(i)
                    //             .arg(elementType)
                    //             .arg(element.x, 0, 'f', 2)
                    //             .arg(element.y, 0, 'f', 2);
                } else {
                    // This is the end point - we have all 4 points for cubic Bezier
                    p3 = point;
                    // qDebug() << QString("Point %1: %2 (%3, %4) - End Point")
                    //             .arg(i)
                    //             .arg(elementType)
                    //             .arg(element.x, 0, 'f', 2)
                    //             .arg(element.y, 0, 'f', 2);
                    
                    // Apply De Casteljau's algorithm at t=0.5 (midpoint of curve)
                    QPointF midpoint = deCasteljau(p0, p1, p2, p3, 0.5);
                    // qDebug() << QString("  -> De Casteljau at t=0.5: (%1, %2)")
                    //             .arg(midpoint.x(), 0, 'f', 2)
                    //             .arg(midpoint.y(), 0, 'f', 2);
                    
                    // Evaluate at multiple t values
                    // qDebug() << "  -> Curve evaluation:";
                    for (qreal t = 0.0; t <= 1.0; t += 0.25) {
                        QPointF evalPoint = deCasteljau(p0, p1, p2, p3, t);
                        // qDebug() << QString("     t=%1: (%2, %3)")
                        //             .arg(t, 0, 'f', 2)
                        //             .arg(evalPoint.x(), 0, 'f', 2)
                        //             .arg(evalPoint.y(), 0, 'f', 2);
                    }
                    
                    // Update current point and p0 for next curve
                    currentPoint = p3;
                    p0 = p3;
                }
                break;
        }
    }
    qDebug() << "=== Total Points:" << m_textPath.elementCount() << "===";
}
