#include <qtToPhotoshopAPI.h>
#include <stdexcept>
#include <QDebug>

using ChannelID = Enum::ChannelID;

TextDepthUnits psApiToQt(const PsApiLayersDto& psData, int width, int height)
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

    TextDepthUnits qtData;
    qtData.reserve(psData.size());
    for (const auto& text : psData) {
        TextDepthUnit qtText;
        qtText.front.baseLayer = toImage(text.front.baseLayer);
        qtText.back.baseLayer = toImage(text.back.baseLayer);
        for (const auto& layer : text.front.clippedLayers) {
            qtText.front.clippedLayers.push_back(toImage(layer));
        }
        for (const auto& layer : text.back.clippedLayers) {
            qtText.back.clippedLayers.push_back(toImage(layer));
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

        qtText.front.vectorMaskData = path;
        qtData.push_back(std::move(qtText));
    }
    return qtData;
}

PsApiLayersDto qtToPsApi (TextDepthUnits qtData) {
    // TODO: Reimplement me! Broken because data type of frontText.vectorMaskData changed from std::vector<std::vector<QPainterPath::Element>> to QPainterPath
    using PathPoint = ImageLayer<bpp8_t>::PathPoint;
    using SubPath = ImageLayer<bpp8_t>::SubPath;
    using Point2D = Geometry::Point2D<int>;

    PsApiLayersDto psData;

    // Go through each text
    for (const auto & textData : qtData )
    {
        // Parse front text
        const auto & frontText = textData.front;
        QPainterPath qtVectorMask = frontText.vectorMaskData;

        std::vector<SubPath> psSubPaths;
        //for (const auto& qtSubpath : qtVectorMask) {
        //    std::vector<PathPoint> psPathPoints;

        //    // Some qtPoints, like CurveTo, may be just one part of a larger, bezier point,
        //    // so track them here. We know we have enough data to form a PathPoint once we have "control point 1",
        //    // "control point 2", and the "end point", which correspond to photoshop API's
        //    // "preceding", "anchor", and "leaving" points respectively
        //    std::optional<Point2D> psPrecedingPoint;
        //    std::optional<Point2D> psAnchorPoint;
        //    std::optional<Point2D> psLeavingPoint;
        //    bool readyToPushBezierPoint = false;

        //    for (int i = 0; i < qtSubpath.size(); i ++)
        //    {
        //        const auto& qtPoint = qtSubpath[i];
        //        int x = qtPoint.x;
        //        int y = qtPoint.y;
        //        switch (qtPoint.type) {
        //            case QPainterPath::MoveToElement:
        //                psPathPoints.push_back(PathPoint({x,y}, {x,y},{x,y}, false));
        //                break;
        //            case QPainterPath::LineToElement:
        //                psPathPoints.push_back(PathPoint({x,y}, {x,y},{x,y}, false));
        //                break;

        //            // Bezier points always in this order CurveToElement, CurveToDataElement, CurveToDataElement
        //            // .. coresponding to control point 1, control point 2, and the end point
        //            case QPainterPath::CurveToElement: // Control point 1
        //                psPrecedingPoint = {x,y};
        //                break;
        //            case QPainterPath::CurveToDataElement:
        //                if (i + 1 < qtSubpath.size() && qtSubpath[i+1].type == QPainterPath::CurveToDataElement) { // Control point 2
        //                    psLeavingPoint = {x,y};
        //                } else {
        //                    psAnchorPoint = {x,y}; // Endpoint
        //                    readyToPushBezierPoint = true;
        //                }
        //                break;
        //        }

        //        if (readyToPushBezierPoint && psPrecedingPoint && psAnchorPoint && psLeavingPoint) {
        //            PathPoint bezierPoint({psPrecedingPoint.value().x,
        //                                        psPrecedingPoint.value().y},
        //                                  {psAnchorPoint.value().x,
        //                                        psAnchorPoint.value().y},
        //                                  {psLeavingPoint.value().x,
        //                                        psLeavingPoint.value().y},
        //                                  false);
        //            psPathPoints.push_back(bezierPoint);

        //            readyToPushBezierPoint = false;
        //            psPrecedingPoint = std::nullopt;
        //            psAnchorPoint = std::nullopt;
        //            psLeavingPoint = std::nullopt;
        //        }
        //    }

        //    SubPath psSubPath(psPathPoints, false);
        //    psSubPaths.push_back(psSubPath);
        //}

        PsApiTextData psText;
        {
            PsApiFrontTextData psFront;
            {
                PsApiVectorMask psVMask(psSubPaths, false);
                psFront.vectorMaskData = psVMask;
            }
            psText.front = psFront;
        }

        // Parse back text
        const auto & backTextBase = textData.back.baseLayer;

        PsApiRasterLayerInfo rasterLayer;

        std::unordered_map <Enum::ChannelID, std::vector<bpp8_t>> channel_map;
        int width = backTextBase.width();
        int height = backTextBase.height();

        channel_map[ChannelID::Red] = std::vector<bpp8_t>(width * height, 0u);
        channel_map[ChannelID::Green] = std::vector<bpp8_t>(width * height, 0u);
        channel_map[ChannelID::Blue] = std::vector<bpp8_t>(width * height, 0u);
        channel_map[ChannelID::Alpha] = std::vector<bpp8_t>(width * height, 0u);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                QColor pixelColor = backTextBase.pixelColor(x, y);
                int red   = pixelColor.red();
                int green = pixelColor.green();
                int blue  = pixelColor.blue();
                int alpha = pixelColor.alpha();

                int flatIndex = y * width + x;
                channel_map[ChannelID::Red][flatIndex] = red;
                channel_map[ChannelID::Green][flatIndex] = green;
                channel_map[ChannelID::Blue][flatIndex] = blue;
                channel_map[ChannelID::Alpha][flatIndex] = alpha;
            }
        }

        ImageLayer<bpp8_t>::Params layer_params = {};
        layer_params.name = "BackText";
        layer_params.width = width;
        layer_params.height = height;

        layer_params.center_x = 32;
        layer_params.center_y = 32;

        psText.back.baseLayer = channel_map;

        psData.push_back(psText);
    }

    return psData;
}
