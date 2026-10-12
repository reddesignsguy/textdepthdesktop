#include <qtToPhotoshopAPI.h>
#include <stdexcept>
#include <QDebug>

using ChannelID = Enum::ChannelID;

RenderedTextDepthUnits psApiToQt(const PsApiLayersDto& psData, int width, int height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("psApiToQt requires positive image dimensions");
    }
    const auto pixelCount = static_cast<std::size_t>(width) * height;
    const auto toImage = [=](const PsApiRasterLayerInfo& channels) {
        if (channels.empty()) {
            return QImage{};
        }
        // TODO: It shoulld be allowed to import layers that have dimensions less or greater than the canvas size
        for (auto channel : {ChannelID::Red, ChannelID::Green, ChannelID::Blue}) {
            const auto it = channels.find(channel);
           if (it == channels.end() || it->second.size() != pixelCount) {
               throw std::invalid_argument("psApiToQt requires RGB channels matching the image dimensions");
           }
        }
        const auto alpha = channels.find(ChannelID::Alpha);
        if (alpha != channels.end() && alpha->second.size() != pixelCount) {
            throw std::invalid_argument("psApiToQt alpha channel does not match the image dimensions");
        }
        QImage image(width, height, QImage::Format_ARGB32);
        if (image.isNull()) {
            throw std::runtime_error("psApiToQt could not allocate an image");
        }
        const auto& red = channels.at(ChannelID::Red);
        const auto& green = channels.at(ChannelID::Green);
        const auto& blue = channels.at(ChannelID::Blue);
        for (int y = 0; y < height; ++y) {
            auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                const auto i = static_cast<std::size_t>(y) * width + x;
                row[x] = qRgba(red[i], green[i], blue[i],
                              alpha == channels.end() ? 255 : alpha->second[i]);
            }
        }
        return image;
    };

    RenderedTextDepthUnits qtData;
    qtData.reserve(psData.size());
    for (const auto& text : psData) {
        Front front;
        Back back;
        front.baseLayer = toImage(text.front.baseLayer);
        back.baseLayer = toImage(text.back.baseLayer);
        for (const auto& layer : text.front.clippedLayers) {
            front.clippedLayers.push_back(toImage(layer));
        }
        for (const auto& layer : text.back.clippedLayers) {
            back.clippedLayers.push_back(toImage(layer));
        }

        QPainterPath path;
        for (const auto& subPath : text.front.vectorMaskData.m_subPaths) {
            //std::vector<QPainterPath::Element> elements;
           // const auto append = [&](const auto& point, QPainterPath::ElementType type) {
           //     QPainterPath::Element element;
           //     element.x = point.x;
           //     element.y = point.y;
           //     element.type = type;
           //     elements.push_back(element);
           // };


            //append(point.m_anchor, QPainterPath::MoveToElement);
            if (subPath.m_points.empty()) {
                continue;
            }

            // pt = point, ctrl = control
            const auto & firstPtOfSubpath = subPath.m_points[0];

            const auto& anchorPtOfFirstPt = firstPtOfSubpath.m_anchor;
            const auto& precedingCtrlPtOfFirst = firstPtOfSubpath.m_preceding;
            auto leavingCtrlPtOfLastPt = std::make_unique<Geometry::Point2D<int>>(firstPtOfSubpath.m_leaving);

            path.moveTo(anchorPtOfFirstPt.x, anchorPtOfFirstPt.y);

            for (int i = 1; i < subPath.m_points.size(); i++) {
                const auto& point = subPath.m_points[i];
                qDebug() << "Preceding (" << point.m_preceding.x << "," << point.m_preceding.y << ")"
                         << "Anchor (" << point.m_anchor.x << "," << point.m_anchor.y << ")"
                         << "Leaving (" << point.m_leaving.x << "," << point.m_leaving.y << ")";

                path.cubicTo(leavingCtrlPtOfLastPt->x, leavingCtrlPtOfLastPt->y,
                             point.m_preceding.x, point.m_preceding.y,
                             point.m_anchor.x, point.m_anchor.y);
                leavingCtrlPtOfLastPt = std::make_unique<Geometry::Point2D<int>>(point.m_leaving);
            }

            // 3 points minimum in order to create closed path.. right?
            if (subPath.m_closed && subPath.m_points.size() >= 3) {
                path.cubicTo(leavingCtrlPtOfLastPt->x, leavingCtrlPtOfLastPt->y,
                             precedingCtrlPtOfFirst.x, precedingCtrlPtOfFirst.y,
                             anchorPtOfFirstPt.x, anchorPtOfFirstPt.y);
            }
        }

        front.vectorMaskData = path;
        const bool hasBack = !back.baseLayer.isNull() || !back.clippedLayers.empty();
        qtData.push_back({QUuid::createUuid(), std::move(front),
                          hasBack ? std::optional<Back>(std::move(back)) : std::nullopt, hasBack});
    }
    return qtData;
}

PsApiLayersDto qtToPsApi(const RenderedTextDepthUnits &units)
{
    const auto channels = [](const QImage &image) {
        PsApiRasterLayerInfo result;
        if (image.isNull()) return result;
        for (auto id : {ChannelID::Red, ChannelID::Green, ChannelID::Blue, ChannelID::Alpha})
            result[id].resize(static_cast<size_t>(image.width()) * image.height());
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const auto pixel = image.pixelColor(x, y);
                const auto index = static_cast<size_t>(y) * image.width() + x;
                result[ChannelID::Red][index] = pixel.red();
                result[ChannelID::Green][index] = pixel.green();
                result[ChannelID::Blue][index] = pixel.blue();
                result[ChannelID::Alpha][index] = pixel.alpha();
            }
        }
        return result;
    };
    using Point = Layer<bpp8_t>::PathPoint;
    using SubPath = Layer<bpp8_t>::SubPath;
    using Position = Geometry::Point2D<int>;
    PsApiLayersDto result;
    for (const auto &unit : units) {
        PsApiTextData text;
        std::vector<SubPath> subpaths;
        std::vector<Point> points;
        const auto flush = [&] {
            if (points.empty()) return;
            const bool closed = points.size() > 1 && points.front().m_anchor == points.back().m_anchor;
            if (closed) {
                points.front().m_preceding = points.back().m_preceding;
                points.pop_back();
            }
            subpaths.emplace_back(points, closed);
            points.clear();
        };
        const auto &path = unit.front.vectorMaskData;
        for (int i = 0; i < path.elementCount(); ++i) {
            const auto element = path.elementAt(i);
            const Position anchor{qRound(element.x), qRound(element.y)};
            if (element.isMoveTo()) {
                flush();
                points.emplace_back(anchor, anchor, anchor, false);
            } else if (element.isLineTo()) {
                points.emplace_back(anchor, anchor, anchor, false);
            } else if (element.isCurveTo() && i + 2 < path.elementCount() && !points.empty()) {
                const auto control = path.elementAt(++i);
                const auto end = path.elementAt(++i);
                const Position endPoint{qRound(end.x), qRound(end.y)};
                points.back().m_leaving = anchor;
                points.emplace_back(Position{qRound(control.x), qRound(control.y)}, endPoint, endPoint, false);
            }
        }
        flush();
        text.front.vectorMaskData = PsApiVectorMask(subpaths, false);
        text.front.baseLayer = channels(unit.front.baseLayer);
        for (const auto &image : unit.front.clippedLayers)
            text.front.clippedLayers.push_back(channels(image));
        if (unit.back) {
            text.back.baseLayer = channels(unit.back->baseLayer);
            for (const auto &image : unit.back->clippedLayers)
                text.back.clippedLayers.push_back(channels(image));
        }
        result.push_back(std::move(text));
    }
    return result;
}
