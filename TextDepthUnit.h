#ifndef TEXTDEPTHUNIT_H
#define TEXTDEPTHUNIT_H

#include <QImage>
#include <QUuid>
#include <QPainterPath>
#include <QMetaType>
#include <vector>
#include <optional>

// Application domain model, independent of PhotoshopAPI.
struct Back {
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct Front {
    QPainterPath vectorMaskData;
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

class TextDepthUnit {
    // Stable identity across document snapshots and reordering.
    QUuid id = QUuid::createUuid();

    // Final calculated data for drawing on viewport and exporting to PSD
    Front front;
    Back back;

    // Pre-rendering data
    QPointF back2FrontRelativePos;
    QPointF back2FrontRelativeSize;

    // Misc
    bool backVisible;
};

using TextDepthUnits = std::vector<TextDepthUnit>;
Q_DECLARE_METATYPE(TextDepthUnits)

#endif // TEXTDEPTHUNIT_H
