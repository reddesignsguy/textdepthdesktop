#include "textdepth.h"
#include <QFont>
#include <QFontMetrics>
#include <QDebug>
#include <cmath>
#include <limits>

TextDepth::TextDepth(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_text("T")
{
    // Set default size
    setWidth(1080);
    setHeight(1080);
    
    // Create initial text path and raster data
    updateTextPath();
    createRasterData();

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true); // optional but useful
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
    font.setPixelSize(m_textSize);
    font.setBold(true);
    
    // Calculate text position to center it
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(m_text);
    
    m_textX = (width() - textRect.width()) / 2.0 - textRect.x();
    m_textY = (height() + textRect.height()) / 2.0;

    qDebug() << "height: " << height() ;
    qDebug() << "text rect height: " << height() ;
    qDebug() << "text rect height: " << height() ;
    qDebug() << "text y: " << m_textY ;
    m_textPath.addText(m_textX, m_textY, font, m_text);

    // Set fill rule to WindingFill to properly fill holes in letters like 'e', 'o', 'a'
    m_textPath.setFillRule(Qt::WindingFill);
}

// Merge a new interval into a list of existing intervals
std::vector<TextDepth::Interval> TextDepth::insertInterval(std::vector<Interval> intervals, Interval newInterval)
{
    intervals.push_back(newInterval);
    std::sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) {
        return a.start < b.start;
    });

    std::vector<Interval> merged;
    merged.push_back(intervals[0]);
    for (const auto& interval : intervals)
    {
        // If the current interval's start overlaps with the last interval in our merged intervals,
        // then we can "extend" the last interval by setting its end to be the current interval's end
        if (interval.start <= merged.back().end)
        {
            merged.back().end = std::max(merged.back().end, interval.end);
        }
        // Otherwise, the current interval is completely disjoint from the last interval and thus
        // we can just append it to to our merged intervals
        else
        {
            merged.push_back(interval);
        }
    }
    intervals = merged;
    return intervals;
}

// Find intersection of a horizontal scanline y = scanY with a line segment
bool TextDepth::intersectScanline(double scanY, const QPointF& p1, const QPointF& p2, double& outX)
{
    if ((p1.y() <= scanY && p2.y() >= scanY) || (p2.y() <= scanY && p1.y() >= scanY))
    {
        if (p1.y() == p2.y())
        {
            // Horizontal edge: choose leftmost point
            outX = std::min(p1.x(), p2.x());
        }
        else
        {
            double t = (scanY - p1.y()) / (p2.y() - p1.y());
            outX = p1.x() + t * (p2.x() - p1.x());
        }
        return true;
    }
    return false;
}

void TextDepth::printIntervalsState(std::unordered_map<int, std::vector<TextDepth::Interval>> intervalState)
{
    qDebug("---------------- Interval state ----------------");
    for (auto& [yIndex, intervals] : intervalState)
    {
        qDebug() << "yIndex: "<< yIndex;
        for (auto& interval: intervals)
        {
            qDebug() << "  start: " << interval.start;
            qDebug() << "  end: " << interval.end;
        }
    }

    qDebug("---------------- ----------------- ----------------");
}

// Optimized scanline algorithm: only use y-values where edges exist
std::vector<TextDepth::Quad> TextDepth::getVisibleQuadsOptimized(const std::vector<Quad>& quads, const std::shared_ptr<IPolygon> frontPolygon)
{
    if (quads.empty())
        return {};


    // Step 0.1: Map of scanline y-value to accumulated intervals
    std::unordered_map<int, std::vector<Interval>> accumulatedIntervals;
    std::vector<Quad> visibleQuads;

    int minY = INT_MAX;
    int maxY = INT_MIN;
    for ( const auto& point: frontPolygon->getPoints())
    {
        minY = std::min((int) point.y(), minY);
        maxY = std::max((int) point.y(), maxY);
    }

    for ( const auto& quad: quads)
    {
        for (const auto& point : quad.getPoints())
        {
            minY = std::min((int) point.y(), minY);
            maxY = std::max((int) point.y(), maxY);
        }
    }

    int numScanlines = 2;

    // Step 0.2: Add scanline intervals of the front polygon
    accumulatedIntervals = getScanlineIntervals(frontPolygon, minY, maxY, numScanlines);
    printIntervalsState(accumulatedIntervals);
    qDebug() << "Accumulated intervals has been populated by intervals of the front polygon";
    // Step 1: Iterate quads from front to back
    for (int quadIndex = quads.size() - 1; quadIndex >= 0; quadIndex--)
    {
        const auto& quad = quads[quadIndex];
        const auto quadPtr = std::make_shared<Quad>(quad);
        bool exposed = false;

        auto polygonIntervals = getScanlineIntervals(quadPtr, minY, maxY, numScanlines);

        // For every y-value
        //    For every intersection
        //         If the intersection is NOT fully engulfed by at least one of the accmulated intervals at this y-value
        //				Then this polygon is exposed
        // 				Also, We can update the accumulated intervals at this y-value

        for (auto& [yIndex, intervals] : polygonIntervals)
        {
            auto& accumulated = accumulatedIntervals[yIndex];

            //    For every intersection
            //         If the intersection is NOT fully engulfed by at least one of the accmulated intervals at this y-value
            //				Then this polygon is exposed
            // 				Also, We can update the accumulated intervals at this y-value
            for (auto& interval : intervals)
            {

                qDebug() << "start: " << interval.start << ", end: " << interval.end;
                bool engulfed = false;
                for (const auto& acc : accumulated)
                {
                    if (interval.start >= acc.start && interval.end <= acc.end)
                    {
                        printIntervalsState(accumulatedIntervals);
                        engulfed = true;
                        break;
                    }
                }

                // This means that this interval is visible to the viewer..
                // and thus this quad should be marked as visible for rendering.
                // We should also merge this interval in as it expands the coverage of our accumulated intervals
                // Future intervals will now consider this interval and see if they are occluded by this interval
                if (!engulfed)
                {
                    exposed = true;

                    printIntervalsState(accumulatedIntervals);
                    // DOUBLE CHECK ME
                    // Do I need to sort before merge interval?
                    accumulatedIntervals[yIndex] = insertInterval(accumulated, interval);
                    printIntervalsState(accumulatedIntervals);
                }
            }
        }

        if (exposed)
        {
            qDebug() << "quad is exposed!";
            visibleQuads.push_back(quad);
        }
        else
        {
            qDebug() <<  "Hiding quad! " << quadIndex;
        }
    }

    return visibleQuads;
}

std::unordered_map<int, std::vector<TextDepth::Interval>> TextDepth::getScanlineIntervals(std::shared_ptr<IPolygon> polygon, int minY, int maxY, int numScanlines){
    qDebug() << "Getting scanline intervals:";
        std::unordered_map<int, std::vector<TextDepth::Interval>> scanlineIntervals;

        // TODO: Put this further upstream? User may want to inject their own value
        // I.  get min and max Y of this quad then use that to find scan lines
        int increment = (maxY - minY) / numScanlines;

        // II. iterate thru scanlines
        int exposeCount = 0;
        for (int scanY = minY; scanY < maxY ; scanY+= increment)
        {
            qDebug() << "   scan Y: " << scanY;
            std::vector<double> intersections;

            // Step 1a.i: Find intersections with edges
            for (const auto& edge : polygon->getEdges())
            {
                double x;
                // TODO: Verify intersection algorithm
                if (intersectScanline(scanY, edge.first, edge.second, x))
                {
                    intersections.push_back(x);
                }
            }

            if (intersections.empty())
            {
                qDebug() << "      NO intersection for scanline";
                continue;
            }

            // TODO: If intersections size is 1, then.. this breaks!
            // Would this even count as exposed? hm.. let's ignore for now.
            if (intersections.size() == 1)
            {
                qDebug() << "       found only one intersection for scanline: " << scanY;
                continue;
            }

            bool numIntersectionsIsOdd = intersections.size() % 2 == 1;
            if (numIntersectionsIsOdd)
            {
                qDebug() << "      intersection size is ODD for y = " << scanY << " with intersetion count = " << intersections.size();
                continue;
            }


            std::sort(intersections.begin(), intersections.end());

            qDebug() << "        intersections size: " << intersections.size() ;
            for (int i = 0; i + 1 < intersections.size(); i += 2)
            {
                // DEBUG: If i + 1 >= intersections.size(), we will be out of bounds...
                // This shouldn't happen because we do a "continue" above if the intersection size is ODD..
                // But this is a fallback
                {
                    qDebug() << "       intersection index: " << i;
                    if (i + 1 >= intersections.size())
                    {

                        qDebug() << "     WARNING: intersection index: " << i << " will be out of bounds!";
                    }
                }

                Interval interval{ intersections[i], intersections[i + 1] };
                scanlineIntervals[scanY].push_back(interval);
            }
        }
        return scanlineIntervals;
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
    smallerFont.setPixelSize(m_textSize * 0.8); // 80% of 80
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
    auto tmp_frontSubpaths = extractPathPoints(m_textPath, 25);
    auto tmp_backSubpaths = extractPathPoints(smallerTextPath, 25);
   // Calculate the vanishing point using the leftmost and rightmost subpaths
   m_vanishingPoint = calculateVanishingPoint(tmp_frontSubpaths, tmp_backSubpaths);
    std::vector<std::vector<Quad>> letterToQuads;
    //std::vector<Quad> quads;
    // Abstract the points into quads which is needed for when we sort them
    for (int subpathIdx = 0; subpathIdx < tmp_frontSubpaths.size(); subpathIdx ++)
    {
        std::vector<QPointF>& frontPoints = tmp_frontSubpaths[subpathIdx];
        std::vector<QPointF>& backPoints = tmp_backSubpaths[subpathIdx];

        std::vector<Quad> quadsForThisLetter;
        for (int i = 0; i < frontPoints.size() - 1; i ++)
        {
            Quad q;
            q.front1 = frontPoints[i];
            q.front2 = frontPoints[i + 1];
            q.back1 = backPoints[i];
            q.back2 = backPoints[i+1];
            // subpathQuads.push_back(q); TODO: testing
            quadsForThisLetter.push_back(q);
        }

        qDebug() << "apatriawan num quads: " << quadsForThisLetter.size() << " for subpathIdx: " << subpathIdx;

        letterToQuads.push_back(quadsForThisLetter);
    }

    // Get all the quads and group each of them with other quads belonging to the same letter
    // Sort the quads within each letter
    // Go thru each letter
    //     0. Determine the min and max Y value which will be the range the scanlines will check for.
    //        Also, determine the # of scanlines to use.
    //     0.1. Initialize a std::unordered_map<int, std::vector<int>> scanlineToListOfAccumulatedIntersections
    //          listOfIntersections is a std::vector<int>
    //     0.2. Initialize a list of quads which represent the final list of quads to be drawn (it will always be a subset of the current
    //           list of quads)
    //     1. Iterate through every quad from front to back (edge case: add in the polygon of the front letter too as it will have a say
    //        in what is concealed or not... that said, technically we're not just dealing with quadilaterals right??)
    //          - Initialize variable "exposed", which is false by default. If any part of the letter is exposed, this will be set to true.
    //            If this letter is exposed, this means we will draw it (and not remove it).
    //          a. For every scan line
    //                  - Initialize a list of intersections
    //					i. Go through every edge in the quad and find the intersections of the scanline; append to list of intersections
    //                     The # of intersections should be even in theory right??
    //                  ii. For every 2 intersections which form an interval, see if this interval is completely overlapped by any of the "accumulated"
    //                      intervals for this scan line.
    //                            A) If any of the intervals ARENT completely overlapped by the accumulated interval, that means this part of the letter
    //                               is EXPOSED (not concealed). Set "exposed" to true for this letter.
    //                               Also, expand the accumulated interval based on this current interval. (Use merge intervals)
    //								 Now intervals for future quads will know if the interval for this quad conceals them or not
    //          b. If exposed is true, then add this quad to the final list of quads to be drawn
    //
    for (int index = 0; index < letterToQuads.size(); index ++)
    {
        std::vector<std::pair<int, int>> subpathQuadsDistToVanishingPointIndices;
        int i = 0;

        // Calculate dist to vanishing points for each quad
        auto& quads = letterToQuads[index];
        for (const auto& quad : quads)
        {
            QPointF avg(0, 0);
            avg += quad.front1;
            avg += quad.front2;
            avg += quad.back1;
            avg += quad.back2;
            avg /= 4;

            subpathQuadsDistToVanishingPointIndices.push_back(std::pair<int,int>{i,
                                                        QLineF(avg, m_vanishingPoint).length()});
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
            Quad quad = quads[index];
            sortedQuads.push_back(quad);
        }

        quads = sortedQuads;
    }

    for (int i = 0; i < letterToQuads.size(); i ++)
    {
        // This polygon is the one for the front text which is always at the front
        std::shared_ptr<Polygon> frontPolygon = std::make_shared<Polygon>();
        frontPolygon->points = tmp_frontSubpaths[i];

    qDebug() << "abany";
        letterToQuads[i] = getVisibleQuadsOptimized(letterToQuads[i], frontPolygon);
    }
    // Remove concealed quads ------------------------------------------
    // do scanlines from the top most point to the bottom X amount of times
    // (we define X -- the amount of scanlines between the top and bottom)


    // Draw letters from the sides towards the center!
    int left = 0;
    int right = letterToQuads.size() - 1;
    while (left <= right)
    {
        qDebug() << "renderLeft";
        const auto& leftQuads = letterToQuads[left];
        renderQuads(leftQuads);
        if (left == right)
        {
            break;
        }

        qDebug() << "renderRight";

        qDebug() << "right = " << right;
        const auto& rightQuads = letterToQuads[right];
        renderQuads(rightQuads);

        left ++;
        right --;
    }
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

void TextDepth::fillPolygonScanline(const std::vector<QPointF>& points, QColor& color)
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
    
    // std::unordered_map<qreal, std::vector<qreal>> pixelsYtoX;
    // For each scanline, find the pixel coords to fill in
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
            int startX = qMax(0, static_cast<int>(qFloor(intersections[i])));
            int endX = qMin(m_rasterImage.width() - 1, static_cast<int>(qFloor(intersections[i + 1])));
            
            for (int x = startX; x <= endX; ++x) {

                qreal alpha = 255;
                // if (x == startX /*|| x == startX + 1*/) {
                //     qreal distToIntersection = qAbs(intersections[i] - x);
                //     qreal coverage = 0.5 - distToIntersection;
                //     coverage = qBound(0.0, coverage, 1.0);
                //     alpha = coverage * 255.0 + 0.5;
                // } else if (x == endX /*|| x == endX - 1*/) {
                //     qreal distToIntersection = qAbs(intersections[i+1] - x);
                //     qreal coverage = 0.5 - distToIntersection;
                //     coverage = qBound(0.0, coverage, 1.0);
                //     alpha = coverage * 255.0 + 0.5;
                // }

                // color.setAlpha(alpha);
                m_rasterImage.setPixel(x, y, color.rgba());
                // pixelsYtoX[y].push_back(x);
            }
        }
    }

    // For each pixel, fill it in
    // for (const auto& [y, xCoords] : pixelsYtoX)
    // {
    //     m_rasterImage.setPixel(x, y, color.rgba());
    // }

}

QPointF TextDepth::closestPointOnLineSegment(
    const QPointF& segmentStart,
    const QPointF& segmentEnd,
    const QPointF& queryPoint)
{
    QPointF segmentDirection = segmentEnd - segmentStart;
    QPointF startToQuery     = queryPoint - segmentStart;

    double segmentLengthSquared =
        segmentDirection.x() * segmentDirection.x() +
        segmentDirection.y() * segmentDirection.y();

    // Degenerate segment (start == end)
    if (segmentLengthSquared == 0.0)
        return segmentStart;

    double projectionFactor =
        (startToQuery.x() * segmentDirection.x() +
         startToQuery.y() * segmentDirection.y()) /
        segmentLengthSquared;

    projectionFactor = qBound(0.0, projectionFactor, 1.0);

    return segmentStart + segmentDirection * projectionFactor;
}

const void TextDepth::fillQuadInRaster(Quad& quad, QColor& color)
{
      //  qDebug("fill quad in raster!");
    const auto& points = quad.getPoints();

    // Find bounding box
    qreal minY = points[0].y();
    qreal maxY = points[0].y();
     //   qDebug("getting mins and maxes!");

    for (const QPointF& p : points) {
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }

    //qDebug("setting start Y and end Y");
    int startY = qMax(0, static_cast<int>(qFloor(minY)));
    int endY = qMin(m_rasterImage.height() - 1, static_cast<int>(qCeil(maxY)));

    //qDebug("loopin");
    // For each scanline, find the pixel coords to fill in
    // This loop scales in time complexity based on the text height
    for (int y = startY; y <= endY; ++y) {

//        qDebug("getting intersdctions!");
        std::vector<qreal> intersections;

        for (auto edge : quad.getEdges())
        {
            const QPointF& p1 = edge.first;
            const QPointF& p2 = edge.second;

            if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y)) {
                // Calculate x intersection
                qreal t = (y - p1.y()) / (p2.y() - p1.y());
                qreal x = p1.x() + t * (p2.x() - p1.x());
                intersections.push_back(x);
            }
        }

        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
//        qDebug("made it!");
//        qDebug() << "# intersections : " << intersections.size();
        // Fill between pairs of intersections
        for (int i = 0; i + 1 < intersections.size(); i += 2) {
//            qDebug() << "i = " << i;
//            qDebug() << "intersections.size() - 1" << intersections.size() - 1;
//            qDebug() << "i < intersection count?? " << (i < intersections.size() - 1);
//            qDebug() << "filling intersections";
            double intersect1 = intersections[i];
            double intersect2 = intersections[i+1];
            int startX = qMax(0, static_cast<int>(qFloor(intersect1)));
            int endX = qMin(m_rasterImage.width() - 1, static_cast<int>(qFloor(intersect2)));

            for (int x = startX; x <= endX; ++x) {
                // if (x == startX /*|| x == startX + 1*/) {
                //     qreal distToIntersection = qAbs(intersections[i] - x);
                //     qreal coverage = 0.5 - distToIntersection;
                //     coverage = qBound(0.0, coverage, 1.0);
                //     alpha = coverage * 255.0 + 0.5;
                // } else if (x == endX /*|| x == endX - 1*/) {
                //     qreal distToIntersection = qAbs(intersections[i+1] - x);
                //     qreal coverage = 0.5 - distToIntersection;
                //     coverage = qBound(0.0, coverage, 1.0);
                //     alpha = coverage * 255.0 + 0.5;
                // }
                QColor under = m_rasterImage.pixelColor(x, y);
                QColor over  = color;

                // Normalize
                float aA = over.alphaF();
                float aB = under.alphaF();

                float outA = aA + aB * (1.0f - aA);

                if (outA <= 0.0f) {
                    m_rasterImage.setPixelColor(x, y, QColor(0, 0, 0, 0));
                    continue;
                }

                float r =
                    (over.redF()   * aA +
                     under.redF()  * aB * (1.0f - aA)) / outA;

                float g =
                    (over.greenF() * aA +
                     under.greenF()* aB * (1.0f - aA)) / outA;

                float b =
                    (over.blueF()  * aA +
                     under.blueF() * aB * (1.0f - aA)) / outA;

                QColor out;
                out.setRgbF(r, g, b, outA);

                m_rasterImage.setPixelColor(x, y, out);
                // pixelsYtoX[y].push_back(x);
            }
        }


      //  qDebug() << "done with this Y";
    }
    // Create a quadrilateral from 4 points and fill it
   // std::vector<QPointF> quad = {p0, p1, p2, p3};
   // fillPolygonScanline(quad, color);
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
    int maxR = 30, maxG = 72, maxB = 120;
    // 40, 96, 160, 255
    
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
    
    const auto& frontPolygon = frontSubpaths[subpathIdx];
    const auto& backPolygon = backSubpaths[subpathIdx];
    
   // qDebug() << "=== Vanishing Point Calculation ===";
   // qDebug() << "Using subpath index:" << subpathIdx;
   // qDebug() << "Front points:" << frontPolygon.size();
   // qDebug() << "Back points:" << backPolygon.size();
    
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

}

void TextDepth::paint(QPainter *painter)
{

   // painter->scale(m_zoom, m_zoom);
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    painter->fillRect(0, 0, width(), height(), QColor(240, 240, 240));
    
    if (m_textPath.isEmpty()) {
        return;
    }

    // Create smaller duplicate text path (80% of original size)
    QFont smallerFont;
    smallerFont.setPixelSize(m_textSize * 0.8); // 80% of 80
    smallerFont.setBold(true);
    
    QFontMetrics smallerMetrics(smallerFont);
    QRect smallerTextRect = smallerMetrics.boundingRect(m_text);
    
    qreal smallerX = (width() - smallerTextRect.width()) / 2.0 - smallerTextRect.x();
    qreal smallerY = (height() + smallerTextRect.height()) / 2.0;
    
    QPainterPath smallerTextPath;
    smallerTextPath.addText(smallerX, smallerY, smallerFont, m_text);
    // smallerTextPath.setFillRule(Qt::WindingFill);
    
    // LAYER 1: Draw smaller duplicate text first (bottom layer)
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 96, 160));
    // painter->drawPath(smallerTextPath);
    
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
    painter->setBrush(QColor(50, 120, 200, 200));
    painter->drawPath(m_textPath);
    
    // LAYER 4: Draw vanishing point as a visible dot
    // if (!m_vanishingPoint.isNull() && m_vanishingPoint.x() != 0 && m_vanishingPoint.y() != 0) {
    //     // Draw a bright red dot for the vanishing point
    //     painter->setPen(Qt::NoPen);
    //     painter->setBrush(QColor(255, 0, 0, 255)); // Bright red
        
    //     // Draw a circle with radius 8 pixels
    //     qreal radius = 8.0;
    //     painter->drawEllipse(m_vanishingPoint, radius, radius);
        
    //     // Draw a white outline for better visibility
    //     painter->setPen(QPen(QColor(255, 255, 255, 255), 2));
    //     painter->setBrush(Qt::NoBrush);
    //     painter->drawEllipse(m_vanishingPoint, radius, radius);
        
    //     qDebug() << "Drawing vanishing point at:" << m_vanishingPoint;
    // }

    // painter->drawLines(QList<QLineF>{
    //     QLineF(tmp1, tmp2),
    //     QLineF(tmp3, tmp4)
    // });
    
    // Print all bezier points and evaluate curves using De Casteljau's algorithm
    qDebug() << "=== Front Text Bezier Points with De Casteljau Evaluation ===";
}
