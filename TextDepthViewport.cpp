#include "TextDepthViewport.h"

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
void TextDepthViewport::paint(QPainter *painter)
{
//    qDebug() << m_layers.size();
//    for (auto & layer : m_layers) {
//        QPainterPath textPath = QPainterPath();
//
//        if (layer.m_text.isEmpty()) {
//            return;
//        }
//
//        QFont font;
//        font.setPixelSize(layer.m_textSize);
//        font.setBold(true);
//
//        // Calculate text position to center it
//        QFontMetrics metrics(font);
//        auto & m_text = layer.m_text;
//        QRect textRect = metrics.boundingRect(m_text);
//        auto & m_textX = layer.m_textX;
//        auto & m_textY = layer.m_textY;
//        auto & m_textSize = layer.m_textSize;
//
//        m_textX = (width() - textRect.width()) / 2.0 - textRect.x();
//        m_textY = (height() + textRect.height()) / 2.0;
//        textPath.addText(m_textX, m_textY, font, m_text);
//
//        // Set fill rule to WindingFill to properly fill holes in letters like 'e', 'o', 'a'
//        textPath.setFillRule(Qt::WindingFill);
//
//        // painter->scale(m_zoom, m_zoom);
//        painter->setRenderHint(QPainter::Antialiasing);
//
//        // Draw background
//        painter->fillRect(0, 0, width(), height(), QColor(240, 240, 240));
//
//
//        // LAYER 1: Draw smaller duplicate text first (bottom layer)
//        painter->setPen(Qt::NoPen);
//
//        painter->setBrush(QColor(40, 96, 160));
//
//        auto & m_coreShadowLoColor =  layer.m_coreShadowLoColor;
//        auto & m_coreShadowHiColor =  layer.m_coreShadowHiColor;
//        auto & m_rasterCoreShadowHi = layer.m_rasterCoreShadowHi;
//        auto & m_rasterCoreShadowLo = layer.m_rasterCoreShadowLo;
//        auto & m_rasterAtmosphere = layer.m_rasterAtmosphere;
//
//        // LAYER 2: Draw pure raster data (middle layer)
//        if (!m_rasterCoreShadowHi.isNull()) {
//            qreal rasterX = (width() - m_rasterCoreShadowHi.width()) / 2.0;
//            qreal rasterY = (height() - m_rasterCoreShadowHi.height()) / 2.0;
//            painter->drawImage(QPointF(rasterX, rasterY),  m_rasterCoreShadowHi);
//        }
//        if (!m_rasterCoreShadowLo.isNull()) {
//            qreal rasterX = (width() - m_rasterCoreShadowLo.width()) / 2.0;
//            qreal rasterY = (height() - m_rasterCoreShadowLo.height()) / 2.0;
//            painter->drawImage(QPointF(rasterX, rasterY),  m_rasterCoreShadowLo);
//        }
//        if (!m_rasterAtmosphere.isNull()) {
//            qreal rasterX = (width() - m_rasterAtmosphere.width()) / 2.0;
//            qreal rasterY = (height() - m_rasterAtmosphere.height()) / 2.0;
//            painter->drawImage(QPointF(rasterX, rasterY),  m_rasterAtmosphere);
//        }
//
//        // LAYER 3: Draw main text on top (top layer)
//        QLinearGradient gradient(0, textPath.boundingRect().bottom(), 0, textPath.boundingRect().top());
//        gradient.setColorAt(0.0, Qt::blue);
//        gradient.setColorAt(1.0, Qt::cyan);
//
//        auto & m_textPath = layer.m_textPath;
//        painter->setBrush(gradient);
//        painter->drawPath(m_textPath);
//    }
//    // DEBUG: DRAW POINTS
//    // {
//    //      painter->setPen(Qt::NoPen);
//    //     painter->setBrush(QColor(255,0,0, 255));
//    //      qreal radius = 1.0;
//    //     for (auto& letter : tmp_points)
//    //     {
//    //         for (auto& pt : letter)
//    //         {
//    //             painter->drawEllipse(pt, radius, radius);
//    //         }
//    //     }
//    // }
//
//    // LAYER 4: Draw vanishing point as a visible dot
//    // if (!m_vanishingPoint.isNull() && m_vanishingPoint.x() != 0 && m_vanishingPoint.y() != 0) {
//    //     // Draw a bright red dot for the vanishing point
//    //     painter->setPen(Qt::NoPen);
//    //     painter->setBrush(QColor(255, 0, 0, 255)); // Bright red
//
//    //     // Draw a circle with radius 8 pixels
//    //     qreal radius = 8.0;
//    //     painter->drawEllipse(m_vanishingPoint, radius, radius);
//
//    //     // Draw a white outline for better visibility
//    //     painter->setPen(QPen(QColor(255, 255, 255, 255), 2));
//    //     painter->setBrush(Qt::NoBrush);
//    //     painter->drawEllipse(m_vanishingPoint, radius, radius);
//
//    //     qDebug() << "Drawing vanishing point at:" << m_vanishingPoint;
//    // }
//
//    // painter->drawLines(QList<QLineF>{
//    //     QLineF(tmp1, tmp2),
//    //     QLineF(tmp3, tmp4)
//    // });
}
