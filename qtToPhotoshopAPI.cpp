#include <qtToPhotoshopAPI.h>

using ChannelID = Enum::ChannelID;

PsApiData qtToPsApi (QtData qtData) {
    using PathPoint = ImageLayer<bpp8_t>::PathPoint;
    using SubPath = ImageLayer<bpp8_t>::SubPath;
    using Point2D = Geometry::Point2D<int>;

    PsApiData psData;

    // Go through each text
    for (const auto & textData : qtData )
    {
        // Parse front text
        const auto & frontText = textData.front;
        std::vector<std::vector<QPainterPath::Element>> qtVectorMask = frontText.vectorMaskData;

        std::vector<SubPath> psSubPaths;
        for (const auto& qtSubpath : qtVectorMask) {
            std::vector<PathPoint> psPathPoints;

            // Some qtPoints, like CurveTo, may be just one part of a larger, bezier point,
            // so track them here. We know we have enough data to form a PathPoint once we have "control point 1",
            // "control point 2", and the "end point", which correspond to photoshop API's
            // "preceding", "anchor", and "leaving" points respectively
            std::optional<Point2D> psPrecedingPoint;
            std::optional<Point2D> psAnchorPoint;
            std::optional<Point2D> psLeavingPoint;
            bool readyToPushBezierPoint = false;

            for (int i = 0; i < qtSubpath.size(); i ++)
            {
                const auto& qtPoint = qtSubpath[i];
                int x = qtPoint.x;
                int y = qtPoint.y;
                switch (qtPoint.type) {
                    case QPainterPath::MoveToElement:
                        psPathPoints.push_back(PathPoint({x,y}, {x,y},{x,y}, false));
                        break;
                    case QPainterPath::LineToElement:
                        psPathPoints.push_back(PathPoint({x,y}, {x,y},{x,y}, false));
                        break;

                    // Bezier points always in this order CurveToElement, CurveToDataElement, CurveToDataElement
                    // .. coresponding to control point 1, control point 2, and the end point
                    case QPainterPath::CurveToElement: // Control point 1
                        psPrecedingPoint = {x,y};
                        break;
                    case QPainterPath::CurveToDataElement:
                        if (i + 1 < qtSubpath.size() && qtSubpath[i+1].type == QPainterPath::CurveToDataElement) { // Control point 2
                            psLeavingPoint = {x,y};
                        } else {
                            psAnchorPoint = {x,y}; // Endpoint
                            readyToPushBezierPoint = true;
                        }
                        break;
                }

                if (readyToPushBezierPoint && psPrecedingPoint && psAnchorPoint && psLeavingPoint) {
                    PathPoint bezierPoint({psPrecedingPoint.value().x,
                                                psPrecedingPoint.value().y},
                                          {psAnchorPoint.value().x,
                                                psAnchorPoint.value().y},
                                          {psLeavingPoint.value().x,
                                                psLeavingPoint.value().y},
                                          false);
                    psPathPoints.push_back(bezierPoint);

                    readyToPushBezierPoint = false;
                    psPrecedingPoint = std::nullopt;
                    psAnchorPoint = std::nullopt;
                    psLeavingPoint = std::nullopt;
                }
            }

            SubPath psSubPath(psPathPoints, false);
            psSubPaths.push_back(psSubPath);
        }

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
