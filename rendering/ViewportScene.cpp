#include "ViewportScene.h"
#include "WireframeNode.h"
#include <QPainter>
#include <QQuickWindow>
#include <QSGOpacityNode>
#include <QSGSimpleTextureNode>

namespace {
QImage compositeLayers(const QImage &base, const std::vector<QImage> &clips)
{
    if (base.isNull()) return {};
    QImage image = base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    for (const auto &clip : clips) painter.drawImage(0, 0, clip);
    return image;
}

void appendImage(QQuickWindow *window, QSGNode *parent, const QImage &image)
{
    if (image.isNull()) return;
    auto *texture = window->createTextureFromImage(image);
    if (!texture) return;
    auto *node = new QSGSimpleTextureNode;
    node->setTexture(texture);
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(QRectF(QPointF(), image.size()));
    parent->appendChildNode(node);
}
}

std::vector<UnitImages> prepareUnitImages(const RenderedTextDepthUnits &units)
{
    std::vector<UnitImages> result;
    result.reserve(units.size());
    for (const auto &unit : units) {
        auto front = compositeLayers(unit.front.baseLayer, unit.front.clippedLayers);
        if (!front.isNull() && !unit.front.vectorMaskData.isEmpty()) {
            // Bake the mask once, preserving the original front image and clips.
            QImage masked(front.size(), QImage::Format_ARGB32_Premultiplied);
            masked.fill(Qt::transparent);
            QPainter painter(&masked);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setClipPath(unit.front.vectorMaskData);
            painter.drawImage(0, 0, front);
            painter.end();
            front = std::move(masked);
        }
        QImage back;
        if (unit.back && !unit.back->layers.empty()) {
            const auto &layers = unit.back->layers;
            back = compositeLayers(layers.front(), {layers.begin() + 1, layers.end()});
        }
        result.push_back({unit.id, std::move(front), std::move(back)});
    }
    return result;
}

void ViewportScene::setUnits(QQuickWindow *window, const std::vector<UnitImages> &units)
{
    m_activeWireframe = nullptr;
    m_units.clear();
    while (auto *child = firstChild()) {
        removeChildNode(child);
        delete child;
    }
    for (const auto &unit : units) {
        auto *group = new QSGNode;
        appendChildNode(group);
        auto *back = new QSGOpacityNode;
        group->appendChildNode(back);
        appendImage(window, back, unit.back);
        auto *preview = new QSGOpacityNode;
        preview->setOpacity(0);
        group->appendChildNode(preview);
        auto *wireframe = new WireframeNode;
        preview->appendChildNode(wireframe);
        appendImage(window, group, unit.front);
        m_units.push_back({unit.id, back, preview, wireframe});
    }
}

void ViewportScene::setPreview(const BackPreview &preview, qreal width)
{
    m_activeWireframe = nullptr;
    for (auto &unit : m_units) {
        const bool active = !preview.unitId.isNull() && unit.id == preview.unitId;
        unit.back->setOpacity(active ? 0 : 1);
        unit.preview->setOpacity(active ? 1 : 0);
        if (active) {
            unit.wireframe->setPath(preview.wireframe, width);
            m_activeWireframe = unit.wireframe;
        } else if (unit.wireframe->geometry()->vertexCount() != 0) {
            unit.wireframe->setPath({}, width);
        }
    }
}

void ViewportScene::setPreviewColor(QColor color)
{
    if (m_activeWireframe) m_activeWireframe->setColor(color);
}
