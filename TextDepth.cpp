#include "TextDepth.h"
#include "BackGeneration.h"
#include <QSet>
#include "PhotoshopWriter.h"
#include <QFont>
#include <QFontMetrics>
#include <QDebug>
#include <QFile>
#include <cmath>
#include <algorithm>
#include <limits>
#include <exception>
#include <filesystem>
#include <utility>
#include "qtToPhotoshopAPI.h"

TextDepth::TextDepth(qreal width, qreal height, QObject *parent) : m_width(width), m_height(height), QObject(parent)
{
}

// This is ChatGPT Generated
bool TextDepth::loadPsd(const QUrl &fileUrl)
{
    if (!fileUrl.isLocalFile()) {
        emit psdLoadFailed(tr("Please choose a local PSD file."));
        return false;
    }

    // ChatGPT Explanation:
    // PhotoshopAPI memory-maps its input. Reject missing/truncated headers before
    // handing it a file, since those can fail before the parser throws an error.
    QFile input(fileUrl.toLocalFile());
    if (!input.open(QIODevice::ReadOnly)) {
        emit psdLoadFailed(tr("Could not open the PSD: %1").arg(input.errorString()));
        return false;
    }
    input.close();

    try {
        const auto path = std::filesystem::path(fileUrl.toLocalFile().toStdString());
        auto layeredFile = LayeredFile<bpp8_t>::read(path);

        // 1. Find "targets" group containing our text group layers
        auto targets = find_layer_as<bpp8_t, GroupLayer>("targets", layeredFile);
        if (!targets) {
            emit psdLoadFailed(tr("The PSD must contain a group named 'targets'."));
            return false;
        }

        PsApiLayersDto extracted;
        for (const auto &layer : targets->layers()) {
            // 2. Every group within "targets" is considered a "text"; Anything other than that will be ignored and lost.
            const auto groupLayer = std::dynamic_pointer_cast<GroupLayer<bpp8_t>>(layer);
            if (!groupLayer) {
                qDebug() << "Warning: 'targets' group contained a non-group layer '" << layer->name() << "'. Ignoring.";
                continue;
            }

            // 3. Get each component of the text.. i.e:
            // The "front text" comprising of an image layer with a vector mask and any clipping masks on it
            // The "back text" comprising of an image layer with any clipping masks on it
            // Anything other than that is ignored and lost.
            for (const auto& layer : groupLayer->layers()) {
                const auto imageLayer = std::dynamic_pointer_cast<ImageLayer<bpp8_t>>(layer);
                // layer->clipping_mask()
                if (!imageLayer || !imageLayer->m_vectorMask) {
                    qDebug() << "Warning: group layer '" << groupLayer->name() << "' contained an invalid layer '" << layer->name() << "'. Ignoring.";
                    continue;
                }
                qDebug() << "albany is cool" << layer->name();
                PsApiTextData textData;
                textData.front.vectorMaskData = *imageLayer->m_vectorMask;
                using chan = Enum::ChannelID;
                textData.front.baseLayer[chan::Red] = imageLayer->get_channel(chan::Red);
                textData.front.baseLayer[chan::Green] = imageLayer->get_channel(chan::Green);
                textData.front.baseLayer[chan::Blue] = imageLayer->get_channel(chan::Blue);
                textData.front.baseLayer[chan::Alpha] = imageLayer->get_channel(chan::Alpha);

                extracted.push_back(std::move(textData));
            }
        }

        // Photoshop lists topmost layers first; the document stores paint order.
        std::reverse(extracted.begin(), extracted.end());
        const auto imported = psApiToQt(extracted, m_width, m_height);
        TextDepthUnits units;
        for (const auto &rendered : imported) {
            TextDepthUnit unit;
            unit.id = rendered.id;
            unit.front = rendered.front;
            unit.back = rendered.back;
            unit.backVisible = unit.back.has_value();
            units.push_back(std::move(unit));
        }
        setUnits(std::move(units));
    } catch (const std::exception &error) {
        emit psdLoadFailed(tr("Could not load the PSD: %1").arg(QString::fromUtf8(error.what())));
        return false;
    }

    return true;
}

void TextDepth::setUnits(TextDepthUnits units)
{
    QSet<QUuid> ids;
    for (auto &unit : units) {
        if (unit.id.isNull() || ids.contains(unit.id))
            unit.id = QUuid::createUuid();
        ids.insert(unit.id);
    }
    m_units = std::move(units);
    publishQtData();
}

void TextDepth::moveUnit(int from, int to)
{
    const int count = static_cast<int>(m_units.size());
    if (from < 0 || to < 0 || from >= count || to >= count || from == to)
        return;
    if (from < to)
        std::rotate(m_units.begin() + from, m_units.begin() + from + 1, m_units.begin() + to + 1);
    else
        std::rotate(m_units.begin() + to, m_units.begin() + from, m_units.begin() + from + 1);
    publishQtData();
}

bool TextDepth::handleToggleUnitVisibility(const QUuid &id)
{
    auto unit = std::find_if(m_units.begin(), m_units.end(), [&](const auto &candidate) {
        return candidate.id == id;
    });
    if (unit == m_units.end())
        return false;
    const bool visible = !unit->backVisible;
    if (visible && !unit->back) {
        try {
            unit->back = generateBack(unit->front, QSize(m_width, m_height),
                                      unit->back2FrontRelativePos, unit->back2FrontRelativeSize);
        } catch (const std::exception &error) {
            emit unitEditFailed(tr("Could not generate back: %1").arg(QString::fromUtf8(error.what())));
            return false;
        }
    }
    unit->backVisible = visible;
    publishQtData();
    return true;
}

RenderedTextDepthUnits TextDepth::renderedUnits() const
{
    RenderedTextDepthUnits rendered;
    rendered.reserve(m_units.size());
    for (const auto &unit : m_units) {
        rendered.push_back({unit.id, unit.front,
                           unit.backVisible ? unit.back : std::nullopt, unit.back.has_value()});
    }
    return rendered;
}

void TextDepth::publishQtData()
{
    emit notifyNewQtData(renderedUnits());
}

// TODO: This is just for testin
void TextDepth::setText(const QString &text)
{
    //if (m_layers.size() > 0)
    //{
    //    m_layers[0].m_text = text;
    //    updateTextPath(m_layers[0]);
    //    createRasterData(m_layers[0]);
    //    emit textChanged();
    //    // update(); // Trigger repaint
    //}
}

void TextDepth::updateTextPath(TextLayerData &data)
{
    //data.m_textPath = QPainterPath();

    //if (data.m_text.isEmpty())
    //{
    //    return;
    //}

    //QFont font;
    //font.setPixelSize(data.m_textSize);
    //font.setBold(true);

    //// TODO: Currently we center it, but in the future we wanna be able to customize it
    //QFontMetrics metrics(font);
    //QRect textRect = metrics.boundingRect(data.m_text);

    //data.m_textX = (m_width - textRect.width()) / 2.0 - textRect.x();
    //data.m_textY = (m_height + textRect.height()) / 2.0;
    //data.m_textPath.addText(data.m_textX, data.m_textY, font, data.m_text);

    //// Set fill rule to WindingFill to properly fill holes in letters like 'e', 'o', 'a'
    //data.m_textPath.setFillRule(Qt::WindingFill);

    //data.backTextData.size = data.m_textSize * 0.8;
    //data.backTextData.x = data.m_textX;
    //data.backTextData.y = data.m_textY + 100;
}

// Merge a new interval into a list of existing intervals
std::vector<TextDepth::Interval> TextDepth::insertInterval(std::vector<Interval> intervals, Interval newInterval)
{
    intervals.push_back(newInterval);
    std::sort(intervals.begin(), intervals.end(), [](const Interval &a, const Interval &b)
              { return a.start < b.start; });

    std::vector<Interval> merged;
    merged.push_back(intervals[0]);
    for (const auto &interval : intervals)
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
bool TextDepth::intersectScanline(double scanY, const QPointF &p1, const QPointF &p2, double &outX)
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
    for (auto &[yIndex, intervals] : intervalState)
    {
        qDebug() << "yIndex: " << yIndex;
        for (auto &interval : intervals)
        {
            qDebug() << "  start: " << interval.start;
            qDebug() << "  end: " << interval.end;
        }
    }

    qDebug("---------------- ----------------- ----------------");
}

// Optimized scanline algorithm: only use y-values where edges exist
std::vector<TextDepth::Quad> TextDepth::getVisibleQuadsOptimized(const std::vector<Quad> &quads, const std::shared_ptr<IPolygon> frontPolygon)
{
    if (quads.empty())
        return {};

    // Step 0.1: Map of scanline y-value to accumulated intervals
    std::unordered_map<int, std::vector<Interval>> accumulatedIntervals;
    std::vector<Quad> visibleQuads;

    int minY = INT_MAX;
    int maxY = INT_MIN;
    for (const auto &point : frontPolygon->getPoints())
    {
        minY = std::min((int)point.y(), minY);
        maxY = std::max((int)point.y(), maxY);
    }

    for (const auto &quad : quads)
    {
        for (const auto &point : quad.getPoints())
        {
            minY = std::min((int)point.y(), minY);
            maxY = std::max((int)point.y(), maxY);
        }
    }

    int numScanlines = 50;

    // Step 0.2: Add scanline intervals of the front polygon
    accumulatedIntervals = getScanlineIntervals(frontPolygon, minY, maxY, numScanlines);
    printIntervalsState(accumulatedIntervals);
    qDebug() << "Accumulated intervals has been populated by intervals of the front polygon";
    // Step 1: Iterate quads from front to back
    for (int quadIndex = quads.size() - 1; quadIndex >= 0; quadIndex--)
    {
        const auto &quad = quads[quadIndex];
        const auto quadPtr = std::make_shared<Quad>(quad);
        bool exposed = false;

        auto polygonIntervals = getScanlineIntervals(quadPtr, minY, maxY, numScanlines);

        // For every y-value
        //    For every intersection
        //         If the intersection is NOT fully engulfed by at least one of the accmulated intervals at this y-value
        //				Then this polygon is exposed
        // 				Also, We can update the accumulated intervals at this y-value

        for (auto &[yIndex, intervals] : polygonIntervals)
        {
            auto &accumulated = accumulatedIntervals[yIndex];

            //    For every intersection
            //         If the intersection is NOT fully engulfed by at least one of the accmulated intervals at this y-value
            //				Then this polygon is exposed
            // 				Also, We can update the accumulated intervals at this y-value
            for (auto &interval : intervals)
            {

                qDebug() << "start: " << interval.start << ", end: " << interval.end;
                bool engulfed = false;
                for (const auto &acc : accumulated)
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
            qDebug() << "Hiding quad! " << quadIndex;
        }
    }

    return visibleQuads;
}

std::unordered_map<int, std::vector<TextDepth::Interval>> TextDepth::getScanlineIntervals(std::shared_ptr<IPolygon> polygon, int minY, int maxY, int numScanlines)
{
    qDebug() << "Getting scanline intervals:";
    std::unordered_map<int, std::vector<TextDepth::Interval>> scanlineIntervals;

    // TODO: Put this further upstream? User may want to inject their own value
    // I.  get min and max Y of this quad then use that to find scan lines
    int increment = (maxY - minY) / numScanlines;

    // II. iterate thru scanlines
    int exposeCount = 0;
    for (int scanY = minY; scanY < maxY; scanY += increment)
    {
        qDebug() << "   scan Y: " << scanY;
        std::vector<double> intersections;

        // Step 1a.i: Find intersections with edges
        for (const auto &edge : polygon->getEdges())
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

        qDebug() << "        intersections size: " << intersections.size();
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

            Interval interval{intersections[i], intersections[i + 1]};
            scanlineIntervals[scanY].push_back(interval);
        }
    }
    return scanlineIntervals;
}



void TextDepth::writeToPhotoshop()
{
    PhotoshopWriter writer;
    writer.write("Bloopy.psd", renderedUnits());
}





std::vector<std::vector<QPainterPath::Element>> TextDepth::getOrganizedPath(const QPainterPath &inputPath)
{
    QPainterPath path = inputPath.simplified();
    std::vector<std::vector<QPainterPath::Element>> res;

    QPointF currentPoint;
    QPointF p0, p1, p2, p3;

    std::vector<QPainterPath::Element> subpath;
    for (int i = 0; i < path.elementCount(); ++i)
    {
        QPainterPath::Element element = path.elementAt(i);

        subpath.push_back(element);
        // If the next element is a new subpath, then clear out this subpath's buffer
        if (i + 1 >= path.elementCount() || path.elementAt(i + 1).type == QPainterPath::MoveToElement)
        {
            res.push_back(subpath);
            subpath.clear();
        }
    }

    return res;
}
// TODO: This function does 2 things.. try to reduce
// Thing 1: Goes thru each painterpath element
// Thing 2: The bezier curves are simplified and the output is all non-bezier points

// ... => PhotoshopWriter doesn't use Thing 2 but can do Thing 1!


QPointF TextDepth::closestPointOnLineSegment(
    const QPointF &segmentStart,
    const QPointF &segmentEnd,
    const QPointF &queryPoint)
{
    QPointF segmentDirection = segmentEnd - segmentStart;
    QPointF startToQuery = queryPoint - segmentStart;

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


// DEPRECATED
// QColor TextDepth::calculateColorFromAngle(const QPointF& p1, const QPointF& p2)
//{
//    qreal dx = p2.x() - p1.x();
//    qreal dy = p2.y() - p1.y();
//
//    qreal angle = std::atan2(dy, dx);
//
//    qreal normalizedAngle = std::abs(angle);
//    if (normalizedAngle > M_PI / 2.0) {
//        normalizedAngle = M_PI - normalizedAngle;
//    }
//
//    qreal horizontalness = 1.0 - (normalizedAngle / (M_PI / 2.0));
//
//    int minR = m_coreShadowLoColor.red(), minG = m_coreShadowLoColor.green(), minB = m_coreShadowLoColor.blue();
//    int maxR = m_coreShadowHiColor.red(), maxG = m_coreShadowHiColor.green(), maxB = m_coreShadowHiColor.blue();
//
//    int r = static_cast<int>(minR + (maxR - minR) * (1.0 - horizontalness));
//    int g = static_cast<int>(minG + (maxG - minG) * (1.0 - horizontalness));
//    int b = static_cast<int>(minB + (maxB - minB) * (1.0 - horizontalness));
//
//    return QColor(r, g, b, 255);
//}

QPointF TextDepth::lineIntersection(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4)
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
    if (qAbs(denom) < 1e-10)
    {
        qDebug() << "Lines are parallel, no intersection";
        return QPointF(0, 0);
    }

    qreal t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;

    // Calculate intersection point
    qreal intersectX = x1 + t * (x2 - x1);
    qreal intersectY = y1 + t * (y2 - y1);
    return QPointF(intersectX, intersectY);
}







//void TextDepth::renderQuads(const std::vector<Quad> quads, TextLayerData &data)
//{
//    auto &m_coreShadowLoColor = data.m_coreShadowLoColor;
//    auto &m_coreShadowHiColor = data.m_coreShadowHiColor;
//    //auto &m_rasterCoreShadowHi = data.m_rasterCoreShadowHi;
//    //auto &m_rasterCoreShadowLo = data.m_rasterCoreShadowLo;
//    for (int i = 0; i < quads.size(); i++)
//    {
//        auto quad = quads[i];
//
//        const QPointF &frontP1 = quad.front1;
//        const QPointF &frontP2 = quad.front2;
//
//        // Alpha value calculations for the core shadow "lo" layer
//        int interpolatedCoreShadowLoOpacity = calculateCoreShadowLoOpacityFromAngle(frontP1, frontP2);
//        if (m_invertCoreShadow)
//        {
//            interpolatedCoreShadowLoOpacity = 255 - interpolatedCoreShadowLoOpacity;
//        }
//        QColor interpolatedCoreShadowLoColor = m_coreShadowLoColor;
//        interpolatedCoreShadowLoColor.setAlpha(interpolatedCoreShadowLoOpacity);
//
//        const auto &points = quad.getPoints();
//        qreal minY = points[0].y();
//        qreal maxY = points[0].y();
//
//        for (const QPointF &p : points)
//        {
//            minY = qMin(minY, p.y());
//            maxY = qMax(maxY, p.y());
//        }
//        int startY = qMax(0, static_cast<int>(qFloor(minY)));
//        //int endY = qMin(m_rasterCoreShadowHi.height() - 1, static_cast<int>(qCeil(maxY)));
//
//        // DRAWING ALGORITHM
//        // For each "scanline", a horizontal line that goes down the screen every 1 pixel,
//        std::cout << "minY" << minY << "maxY:" << maxY << std::endl;
//       //  	Find all the points where the scanline intersects with the quad's edges
//       // This loop scales in time complexity based on the text height
//        for (int y = startY; y <= endY; ++y)
//        {
//            std::vector<qreal> intersections;
//
//            for (auto edge : quad.getEdges())
//            {
//                const QPointF &p1 = edge.first;
//                const QPointF &p2 = edge.second;
//
//                if ((p1.y() <= y && p2.y() > y) || (p2.y() <= y && p1.y() > y))
//                {
//                    // Calculate x intersection
//                    qreal t = (y - p1.y()) / (p2.y() - p1.y());
//                    qreal x = p1.x() + t * (p2.x() - p1.x());
//                    intersections.push_back(x);
//                }
//            }
//
//            // Sort intersections
//            std::sort(intersections.begin(), intersections.end());
//
//            // Fill between pairs of intersections
//            for (int i = 0; i + 1 < intersections.size(); i += 2)
//            {
//                double intersect1 = intersections[i];
//                double intersect2 = intersections[i + 1];
//                int startX = qMax(0, static_cast<int>(qFloor(intersect1)));
//                int endX = qMin(m_rasterCoreShadowHi.width() - 1, static_cast<int>(qFloor(intersect2)));
//
//                for (int x = startX; x <= endX; ++x)
//                {
//                    m_rasterCoreShadowHi.setPixelColor(x, y, m_coreShadowHiColor);
//                    m_rasterCoreShadowLo.setPixelColor(x, y, interpolatedCoreShadowLoColor); // Tmp
//                    //  m_rasterAtmosphere.setPixelColor(x, y, QColor(255,0,255)); // Tmp
//                }
//            }
//        }
//    }
//}

void TextDepth::addLayer(const QString name)
{
    qDebug() << "adding layer from textdepth backend!";

    TextLayerData data;
   // data.m_text = name;
    updateTextPath(data);
    //createRasterData(data);
    m_layers.push_back(data);

    for (auto &layer : m_layers)
    {
        continue;
        // resetRasterLayers(layer);
    }
    sendLayersChangeSignal();
}

void TextDepth::sendLayersChangeSignal()
{

    std::vector<TextDrawable> drawables;
    for (auto &layer : m_layers)
    {
        TextDrawable text;

        ImageDrawable img{};

        // push_back(layer.m_rasterCoreShadowHi);
    }
}
