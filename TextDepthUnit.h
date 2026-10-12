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

struct TextDepthUnit {
    // Stable identity across document snapshots and reordering.
    QUuid id = QUuid::createUuid();
    Front front;
    Back back;
    // Retain imported back layers while the generated 3D effect is enabled.
    std::optional<Back> backBefore3D;
    bool is3D() const { return backBefore3D.has_value(); }
};

using TextDepthUnits = std::vector<TextDepthUnit>;
Q_DECLARE_METATYPE(TextDepthUnits)

#endif // TEXTDEPTHUNIT_H
