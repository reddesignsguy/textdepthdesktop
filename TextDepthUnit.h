#ifndef TEXTDEPTHUNIT_H
#define TEXTDEPTHUNIT_H

#include <QImage>
#include <QColor>
#include <QUuid>
#include <QPainterPath>
#include <QMetaType>
#include <vector>
#include <optional>

struct GradientStop {
    double position;
    QColor color;
    QUuid id = QUuid::createUuid();
};
using ShadowGradient = std::vector<GradientStop>;
ShadowGradient defaultShadowGradient();
QColor sampleShadowGradient(const ShadowGradient &stops, double position);
Q_DECLARE_METATYPE(ShadowGradient)

// Backend geometry/shading cache. RGB channels are grayscale; alpha is coverage.
struct Back {
    QImage baseLayer;
    QImage shadowLayer;
};

// Presentation only: layer 0 is the base; subsequent layers draw above it.
struct RenderedBack {
    std::vector<QImage> layers;
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
    const std::optional<RenderedBack> back;
    const bool backAvailable = false;
    const ShadowGradient shadowGradient = defaultShadowGradient();
};

// Mutable document state owned by the backend.
struct TextDepthUnit {
    QUuid id = QUuid::createUuid();
    Front front;
    std::optional<Back> back;
    QPointF back2FrontRelativePos{0, 130};
    QPointF back2FrontRelativeSize{0.8, 0.8};
    bool backVisible = false;
    ShadowGradient shadowGradient = defaultShadowGradient();
};

QImage mapShadowGradient(const QImage &grayscale, const ShadowGradient &stops);
RenderedTextDepthUnit renderUnit(const TextDepthUnit &unit);

using TextDepthUnits = std::vector<TextDepthUnit>;
using RenderedTextDepthUnits = std::vector<RenderedTextDepthUnit>;
Q_DECLARE_METATYPE(RenderedTextDepthUnits)

#endif // TEXTDEPTHUNIT_H
