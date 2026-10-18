#include "TextDepthUnit.h"
#include <array>
#include <algorithm>
#include <cmath>

ShadowGradient defaultShadowGradient()
{
    return {{0.0, QColor(77, 24, 0)}, {1.0, QColor(133, 67, 14)}};
}

QColor sampleShadowGradient(const ShadowGradient &stops, double position)
{
    if (stops.empty()) return Qt::black;
    auto upper = std::upper_bound(stops.begin(), stops.end(), position,
                                 [](double p, const GradientStop &stop) { return p < stop.position; });
    if (upper == stops.begin()) return upper->color;
    if (upper == stops.end()) return stops.back().color;
    const auto &lower = *(upper - 1);
    const double t = (position - lower.position) / (upper->position - lower.position);
    const auto mix = [t](int a, int b) { return qRound(a + (b - a) * t); };
    return QColor(mix(lower.color.red(), upper->color.red()),
                  mix(lower.color.green(), upper->color.green()),
                  mix(lower.color.blue(), upper->color.blue()));
}

QImage mapShadowGradient(const QImage &grayscale, const ShadowGradient &stops)
{
    if (grayscale.isNull()) return {};
    const QImage source = grayscale.convertToFormat(QImage::Format_ARGB32);
    QImage result(source.size(), QImage::Format_ARGB32);
    std::array<QRgb, 256> gradient;
    for (int value = 0; value < 256; ++value)
        gradient[value] = sampleShadowGradient(stops, value / 255.0).rgb();
    for (int y = 0; y < source.height(); ++y) {
        const auto *input = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        auto *output = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < source.width(); ++x)
            output[x] = (gradient[qRed(input[x])] & 0x00ffffff) | (input[x] & 0xff000000);
    }
    return result;
}

RenderedTextDepthUnit renderUnit(const TextDepthUnit &unit)
{
    std::optional<RenderedBack> back;
    if (unit.backVisible && unit.back) {
        back = RenderedBack{{mapShadowGradient(unit.back->baseLayer, unit.shadowGradient),
                             mapShadowGradient(unit.back->shadowLayer, unit.shadowGradient)}};
    }
    return {unit.id, unit.front, std::move(back), unit.back.has_value(),
            unit.shadowGradient};
}
