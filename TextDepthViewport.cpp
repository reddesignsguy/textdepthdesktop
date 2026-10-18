#include "TextDepthViewport.h"
#include <cmath>

TextDepthViewport::TextDepthViewport(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    setClip(true);
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

void TextDepthViewport::setWireframeWidth(qreal width)
{
    if (!std::isfinite(width) || width <= 0 || m_wireframeWidth == width) return;
    m_wireframeWidth = width;
    ++m_previewRevision;
    emit wireframeWidthChanged();
    update();
}

void TextDepthViewport::handleBackPreview(const BackPreview &preview)
{
    if (!preview.unitId.isNull() && m_preview.unitId.isNull()) {
        m_pulseClock.start();
        m_pulseTimer.start();
    }
    m_preview = preview;
    ++m_previewRevision;
    if (preview.unitId.isNull()) m_pulseTimer.stop();
    update();
}

void TextDepthViewport::handleNewQtData(const RenderedTextDepthUnits &units)
{
    m_images = prepareUnitImages(units);
    ++m_documentRevision;
    update();
}

QSGNode *TextDepthViewport::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    // Qt synchronizes item state here with the GUI thread blocked. All GPU
    // resources belong to this tree and are destroyed by Qt on the render thread.
    auto *scene = static_cast<ViewportScene *>(oldNode);
    if (!scene) scene = new ViewportScene;
    if (scene->documentRevision != m_documentRevision) {
        scene->setUnits(window(), m_images);
        scene->documentRevision = m_documentRevision;
        scene->previewRevision = 0;
    }
    if (scene->previewRevision != m_previewRevision) {
        scene->setPreview(m_preview, m_wireframeWidth);
        scene->previewRevision = m_previewRevision;
    }
    if (!m_preview.unitId.isNull()) {
        const double brightness = 0.65 + 0.35 * std::sin(m_pulseClock.elapsed() * 0.005);
        QColor color = m_wireframeColor;
        color.setRedF(color.redF() * brightness);
        color.setGreenF(color.greenF() * brightness);
        color.setBlueF(color.blueF() * brightness);
        // Holding still updates a material, reusing all geometry and textures.
        scene->setPreviewColor(color);
    }
    return scene;
}
