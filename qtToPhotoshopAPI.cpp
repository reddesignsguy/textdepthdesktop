#include <qtToPhotoshopAPI.h>

void PsApiRasterLayerInfo::setChannel(Enum::ChannelID channelID, std::vector<bpp8_t> channel) {
    if (width == -1 || height == -1) {
        throw std::runtime_error("setChannel(): must initialize width and height first with a non-zero value!");
    }

    if (channel.size() != width * height) {
        throw std::runtime_error("setChannel(): the channel passed must have a size equal to width * height!");
    }

    m_channelMap[channelID] = channel;
}

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
                PsApiVectorMask psVMask(psSubPaths);
                psFront.vectorMaskData = psVMask;
            }
            psText.front = psFront;
        }

        psData.push_back(psText);
    }

    return psData;
}
