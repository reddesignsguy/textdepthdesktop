#include "TextDepthViewport.h"
#include <cmath>

TextDepthViewport::TextDepthViewport(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    m_pulseTimer.setInterval(16);
    connect(&m_pulseTimer, &QTimer::timeout, this, [this] { update(); });
}

void TextDepthViewport::setWireframeColor(QColor color)
{
    if (m_wireframeColor == color) return;
    m_wireframeColor = color;
    emit wireframeColorChanged();
    update();
}

void TextDepthViewport::handleBackPreview(const BackPreview &preview)
{
    if (!preview.unitId.isNull() && m_preview.unitId.isNull()) {
        m_pulseClock.start();
        m_pulseTimer.start();
    }
    m_preview = preview;
    if (preview.unitId.isNull()) m_pulseTimer.stop();
    update();
}

void TextDepthViewport::drawLayers(std::vector<TextDrawable> layers)
{
    for (auto &layer : layers) m_items.push_back({{}, std::move(layer)});
    update();
}

namespace {
QImage compositeLayers(const QImage &base, const std::vector<QImage> &clips)
{
    if (base.isNull()) return {};
    QImage image = base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    for (const auto &clip : clips)
        painter.drawImage(0, 0, clip);
    return image;
}
}

void TextDepthViewport::handleNewQtData(const RenderedTextDepthUnits &units)
{
    m_items.clear();
    // Snapshots are in paint order: draw the entire lower unit before the next.
    for (const auto &unit : units) {
        TextDrawable drawable;
        if (unit.back) {
            ImageDrawable back;
            back.image = compositeLayers(unit.back->baseLayer, unit.back->clippedLayers);
            drawable.backTextLayers.push_back(std::move(back));
        }
        drawable.frontText.path = unit.front.vectorMaskData;
        drawable.frontTextBase.image = compositeLayers(unit.front.baseLayer, unit.front.clippedLayers);
        m_items.push_back({unit.id, std::move(drawable)});
    }
    update();
}

void TextDepthViewport::paint(QPainter *painter)
{
    for (auto &item : m_items) {
        if (item.id != m_preview.unitId || m_preview.unitId.isNull()) {
            item.drawable.draw(painter);
            continue;
        }
        painter->save();
        const double brightness = 0.65 + 0.35 * std::sin(m_pulseClock.elapsed() * 0.005);
        QColor color = m_wireframeColor;
        color.setRedF(color.redF() * brightness);
        color.setGreenF(color.greenF() * brightness);
        color.setBlueF(color.blueF() * brightness);
        QPen pen(color, 1.5);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->setRenderHint(QPainter::Antialiasing);
        qDebug() << "b4!" << QTime::currentTime();
        painter->drawPath(m_preview.wireframe);
        qDebug() << "aft!" << QTime::currentTime();
        painter->restore();
        item.drawable.drawFront(painter);
    }
}
