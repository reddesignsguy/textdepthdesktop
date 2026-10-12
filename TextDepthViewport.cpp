#include "TextDepthViewport.h"
#include <QDebug>

TextDepthViewport::TextDepthViewport(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
//    TextLayerData data;
//    m_layers.push_back(data);
//    for (auto & layer : m_layers)
//    {
//        resetRasterLayers(layer);
//    }
    // TODO: Inject this from UI
    qDebug() << Q_FUNC_INFO;
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true); // optional but useful

}
// TODO: This is just for testin
void TextDepthViewport::setText(const QString &text)
{
    //if (m_layers.size() > 0)
    //{
    //    m_layers[0].m_text = text;
    //    updateTextPath(m_layers[0]);
    //    createRasterData(m_layers[0]);
    //    emit textChanged();
    //    update(); // Trigger repaint
    //}
}

void TextDepthViewport::drawLayers(std::vector<TextDrawable> layers) {
    for (auto & layer : layers)
    {
        m_thingsToDraw.push_back(std::make_unique<TextDrawable>(layer));
    }
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
    m_thingsToDraw.clear();
    // Snapshots are in paint order: draw the entire lower unit before the next.
    for (const auto &unit : units) {
        auto drawable = std::make_unique<TextDrawable>();
        if (unit.back) {
            ImageDrawable back;
            back.image = compositeLayers(unit.back->baseLayer, unit.back->clippedLayers);
            drawable->backTextLayers.push_back(std::move(back));
        }
        drawable->frontText.path = unit.front.vectorMaskData;
        drawable->frontTextBase.image = compositeLayers(unit.front.baseLayer, unit.front.clippedLayers);
        m_thingsToDraw.push_back(std::move(drawable));
    }
    update();
}

void TextDepthViewport::paint(QPainter *painter)
{
    for (auto &drawable : m_thingsToDraw)
        drawable->draw(painter);
}
