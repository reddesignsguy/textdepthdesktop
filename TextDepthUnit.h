#ifndef TEXTDEPTHUNIT_H
#define TEXTDEPTHUNIT_H

#include <QImage>
#include <QUuid>
#include <QPainterPath>
#include <QMetaType>
#include <vector>
#include <optional>

struct Back {
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct Front {
    QPainterPath vectorMaskData;
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

// Immutable snapshot for views and export. A hidden back is omitted, even when
// the backend has cached it. Identity and availability support editor controls.
struct RenderedTextDepthUnit {
    const QUuid id;
    const Front front;
    const std::optional<Back> back;
    const bool backAvailable = false;
};

// Mutable document state owned by the backend.
struct TextDepthUnit {
    QUuid id = QUuid::createUuid();
    Front front;
    std::optional<Back> back;
    QPointF back2FrontRelativePos{0, 130};
    QPointF back2FrontRelativeSize{0.8, 0.8};
    bool backVisible = false;
};

using TextDepthUnits = std::vector<TextDepthUnit>;
using RenderedTextDepthUnits = std::vector<RenderedTextDepthUnit>;
Q_DECLARE_METATYPE(RenderedTextDepthUnits)

#endif // TEXTDEPTHUNIT_H
