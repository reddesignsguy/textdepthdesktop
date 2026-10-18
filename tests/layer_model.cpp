#include "LayerUIModel.h"
#include "TextDepth.h"
#include "UnitPropertiesModel.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include "rendering/WireframeNode.h"
#include "TextDepthViewport.h"
#include "qtToPhotoshopAPI.h"
#include "BackGeneration.h"
#include <type_traits>
#include <QPersistentModelIndex>
#include <stdexcept>

static void check(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QSurfaceFormat format;
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    QGuiApplication app(argc, argv);
    TextDepth backend(64, 256);
    LayerUIModel model;
    // A second independent subscriber stands in for the viewport.
    RenderedTextDepthUnits viewportState;
    QObject::connect(&backend, &TextDepth::notifyNewQtData,
                     [&](const RenderedTextDepthUnits &state) { viewportState = RenderedTextDepthUnits(state); });
    QObject::connect(&backend, &TextDepth::notifyNewQtData, &model, &LayerUIModel::setUnits);
    QObject::connect(&model, &LayerUIModel::moveRequested, &backend, &TextDepth::moveUnit);
    check(model.rowCount() == 0 && model.selectedUnitId().isNull(), "empty document");
    check(!model.data({}, LayerUIModel::NameRole).isValid(), "invalid index");
    TextDepthUnits units(3);
    for (int i = 0; i < 3; ++i) {
        units[i].front.baseLayer = QImage(1, 1, QImage::Format_ARGB32);
        units[i].front.baseLayer.fill(QColor(i + 1, 0, 0));
    }
    int publications = 0;
    QObject::connect(&backend, &TextDepth::notifyNewQtData, [&] { ++publications; });
    backend.setUnits(units);
    check(model.rowCount() == 3 && model.rowCount(model.index(0)) == 0, "flat list");
    check(model.data(model.index(0), LayerUIModel::NameRole) == "Layer 1", "numeric label");
    model.selectLayer(0);
    check(model.selectedUnitId() == units[2].id, "top row is frontmost");
    QPersistentModelIndex first(model.index(0));
    check(model.moveLayer(0, 2), "move down");
    check(first.row() == 2 && model.selectedRow() == 2, "selection and persistent index follow unit");
    check(backend.units()[0].front.baseLayer.pixelColor(0, 0).red() == 3, "render order changes");
    check(model.data(model.index(2), LayerUIModel::NameRole) == "Layer 1", "label follows unit");
    check(model.moveLayer(2, 0), "move up");
    check(backend.units()[2].front.baseLayer.pixelColor(0, 0).red() == 3, "render order restored");
    model.selectLayer(1);
    check(model.moveLayer(0, 2) && model.selectedRow() == 0, "selection shifts past moved unit");
    check(model.selectedUnitId() == units[1].id, "same selected unit");
    check(model.moveLayer(2, 0) && model.selectedRow() == 1, "selection shifts in reverse");
    check(viewportState[2].id == backend.units()[2].id, "independent subscribers receive same order");
    backend.publishQtData();
    check(model.selectedUnitId() == units[1].id, "republishing preserves selection");
    // A request alone cannot mutate the presentation; the backend must confirm it.
    LayerUIModel disconnected;
    disconnected.setUnits(backend.renderedUnits());
    disconnected.moveLayer(0, 2);
    check(disconnected.data(disconnected.index(0), LayerUIModel::NameRole) == "Layer 1",
          "presentation cannot mutate document order on its own");
    backend.moveUnit(0, 2);
    check(model.data(model.index(0), LayerUIModel::NameRole) == "Layer 3",
          "direct backend changes propagate without a panel request");
    backend.moveUnit(2, 0);
    const int before = publications;
    check(!model.moveLayer(-1, 0) && !model.moveLayer(0, 3) && !model.moveLayer(0, 0), "invalid moves rejected");
    backend.moveUnit(-1, 0);
    backend.moveUnit(0, 3);
    backend.moveUnit(0, 0);
    model.selectLayer(99);
    check(model.selectedRow() == 1 && publications == before, "invalid actions do not publish");
    model.selectLayer(-1);
    check(model.selectedUnitId().isNull(), "clear selection");
    model.selectLayer(2);
    backend.setUnits(TextDepthUnits(1));
    check(model.selectedRow() == -1 && !first.isValid(), "replacement clears selection and old indices");
    backend.setUnits({});
    check(model.rowCount() == 0 && model.selectedUnitId().isNull(), "clear document");
    static_assert(!std::is_assignable_v<decltype(RenderedTextDepthUnit::front)&, Front>);
    UnitPropertiesModel properties;
    QObject::connect(&backend, &TextDepth::notifyNewQtData, &properties, &UnitPropertiesModel::setUnits);
    QObject::connect(&properties, &UnitPropertiesModel::toggleBackRequested,
                     &backend, &TextDepth::handleToggleUnitVisibility);
    TextDepthUnits effectUnits(2);
    effectUnits[0].front.vectorMaskData.addRect(10, 10, 30, 30);
    const auto effectId = effectUnits[0].id;
    backend.setUnits(effectUnits);
    properties.selectUnit(effectId);
    check(properties.canShowBack() && !properties.backVisible(), "back starts hidden");
    properties.requestToggleBack();
    check(backend.units()[0].backVisible && properties.backVisible(), "visibility propagates");
    check(!backend.units()[1].backVisible && !backend.units()[1].back, "only requested unit changes");
    const auto generated = *backend.units()[0].back;
    check(generated.baseLayer.pixelColor(0, 0).alpha() == 0 &&
          generated.clippedLayers[0].pixelColor(0, 0).alpha() == 0, "generated images start transparent");
    const auto oldSnapshot = backend.renderedUnits();
    backend.moveUnit(0, 1);
    properties.requestToggleBack();
    check(!backend.renderedUnits()[1].back && backend.units()[1].back.has_value(), "hide omits back but retains cache");
    check(oldSnapshot[0].back.has_value(), "published snapshots remain immutable");
    check(qtToPsApi(backend.renderedUnits())[1].back.baseLayer.empty(), "hidden back omitted from export");
    properties.requestToggleBack();
    check(backend.units()[1].back->baseLayer == generated.baseLayer &&
          backend.units()[1].back->clippedLayers == generated.clippedLayers, "show reuses cached back");
    check(!qtToPsApi(backend.renderedUnits())[1].back.baseLayer.empty(), "visible back exported");
    properties.selectUnit(effectUnits[1].id);
    check(!properties.canShowBack() && !properties.backVisible(), "empty mask unavailable");
    check(!backend.handleToggleUnitVisibility(effectUnits[1].id), "empty mask rejected");
    check(!backend.handleToggleUnitVisibility(QUuid::createUuid()), "missing unit rejected");

    // Pixel-level stacking check through the OpenGL scene graph.
    const auto solid = [](QColor color) {
        QImage image(64, 256, QImage::Format_ARGB32);
        image.fill(color);
        return image;
    };
    TextDepthUnits overlapping(2);
    overlapping[0].front.baseLayer = solid(Qt::red);
    overlapping[0].front.vectorMaskData.addRect(0, 0, 64, 256);
    overlapping[1].front.baseLayer = solid(Qt::blue);
    overlapping[1].front.vectorMaskData.addRect(10, 10, 10, 10);
    Back back;
    back.baseLayer = solid(Qt::transparent);
    { QPainter painter(&back.baseLayer); painter.fillRect(5, 5, 20, 20, Qt::green); }
    overlapping[1].back = back;
    const auto topId = overlapping[1].id;
    QQuickWindow window;
    window.setColor(Qt::transparent);
    window.resize(64, 256);
    TextDepthViewport viewport;
    viewport.setParentItem(window.contentItem());
    viewport.setSize(QSizeF(64, 256));
    window.show();
    QCoreApplication::processEvents();
    QObject::connect(&backend, &TextDepth::notifyNewQtData, &viewport, &TextDepthViewport::handleNewQtData);
    const auto render = [&] {
        const auto frame = window.grabWindow();
        check(!frame.isNull(), "OpenGL framebuffer is available");
        check(window.rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL,
              "pixel checks use OpenGL");
        return frame.scaled(64, 256);
    };
    backend.setUnits(overlapping);
    check(render().pixelColor(6, 6) == QColor(Qt::red), "hidden top back does not draw");
    backend.handleToggleUnitVisibility(topId);
    check(render().pixelColor(6, 6) == QColor(Qt::green), "upper back covers lower front");
    check(render().pixelColor(12, 12) == QColor(Qt::blue), "own front covers own back");
    check(render().pixelColor(0, 0) == QColor(Qt::red), "transparent area preserves lower layer");
    backend.moveUnit(1, 0);
    check(render().pixelColor(6, 6) == QColor(Qt::red), "lower back cannot cover upper front");
    backend.handleToggleUnitVisibility(topId);
    check(render().pixelColor(6, 6) == QColor(Qt::red), "toggling lower unit leaves upper unit intact");
    backend.moveUnit(0, 1);
    backend.handleToggleUnitVisibility(topId);
    check(render().pixelColor(6, 6) == QColor(Qt::green), "repeated toggle after reorder selects same unit");
    // A cached back is usable even without geometry for generating a new one.
    overlapping[1].front.vectorMaskData = {};
    backend.setUnits(overlapping);
    properties.selectUnit(topId);
    check(properties.canShowBack(), "cached back can be shown without a vector mask");
    properties.requestToggleBack();
    check(backend.renderedUnits()[1].back->baseLayer == back.baseLayer, "existing back never regenerated");
    auto duplicate = overlapping;
    duplicate[1].id = duplicate[0].id;
    backend.setUnits(duplicate);
    check(backend.units()[0].id != backend.units()[1].id, "duplicate imported IDs cannot cross-toggle units");
    // Preview belongs to its unit's paint slot, never a global overlay.
    overlapping[1].front.vectorMaskData.addRect(10, 10, 10, 10);
    overlapping[1].backVisible = true;
    backend.setUnits(overlapping);
    QPainterPath previewPath;
    previewPath.moveTo(5, 6); previewPath.lineTo(100000, 6);
    viewport.handleBackPreview({topId, previewPath});
    const auto previewPixel = render().pixelColor(12, 6);
    check(previewPixel.green() > previewPixel.red(), "upper wireframe draws above lower front");
    check(render().pixelColor(12, 12) == QColor(Qt::blue), "preview preserves own front");
    check(render().pixelColor(6, 8) == QColor(Qt::red), "preview suppresses cached back raster");
    backend.moveUnit(1, 0);
    check(render().pixelColor(12, 6) == QColor(Qt::red), "upper front occludes lower wireframe");
    viewport.handleBackPreview({});

    // Longer connectors consume the same amount of GPU geometry. Pulsing only
    // changes the material; it must not regenerate or reallocate that geometry.
    WireframeNode wireframe;
    QPainterPath shortLine;
    shortLine.moveTo(0, 0); shortLine.lineTo(10, 10);
    wireframe.setPath(shortLine, 1.5);
    const auto shortCount = wireframe.geometry()->vertexCount();
    QPainterPath longLine;
    longLine.moveTo(0, 0); longLine.lineTo(100000, 100000);
    wireframe.setPath(longLine, 1.5);
    check(shortCount == 6 && wireframe.geometry()->vertexCount() == shortCount,
          "connector length does not increase mesh size");
    const auto *vertices = wireframe.geometry()->vertexData();
    wireframe.setColor(Qt::green);
    wireframe.setColor(Qt::darkGreen);
    check(wireframe.geometry()->vertexData() == vertices, "pulse reuses geometry");

    // A translation emits only lightweight previews until release.
    TextDepthUnits movable(2);
    movable[0].front.vectorMaskData.addRect(10, 10, 30, 30);
    movable[1].front.vectorMaskData.addRect(15, 15, 30, 30);
    backend.setUnits(movable);
    const auto movingId = movable[0].id;
    backend.handleToggleUnitVisibility(movingId);
    const auto cached = backend.units()[0].back->baseLayer;
    const auto originalOffset = backend.units()[0].back2FrontRelativePos;
    BackPreview preview;
    int previews = 0;
    QObject::connect(&backend, &TextDepth::backPreviewChanged, &viewport, &TextDepthViewport::handleBackPreview);
    QObject::connect(&backend, &TextDepth::backPreviewChanged, [&](const BackPreview &value) {
        preview = value;
        ++previews;
    });
    const int beforeDrag = publications;
    check(backend.beginBackTranslation(movingId), "visible back can be dragged");
    backend.translateBack(movingId, {12, -20});
    check(preview.unitId == movingId && !preview.wireframe.isEmpty(), "backend sends transient wireframe");
    check(backend.units()[0].back2FrontRelativePos == originalOffset + QPointF(12, -20), "drag updates domain offset");
    check(backend.units()[0].back->baseLayer == cached && publications == beforeDrag, "drag does not rasterize or publish document");
    check(backend.units()[1].back2FrontRelativePos == movable[1].back2FrontRelativePos, "other unit offset unchanged");
    const auto shifted = backOutline(movable[0].front, originalOffset + QPointF(12, -20), movable[0].back2FrontRelativeSize);
    const auto unshifted = backOutline(movable[0].front, originalOffset, movable[0].back2FrontRelativeSize);
    check(shifted.boundingRect().topLeft() - unshifted.boundingRect().topLeft() == QPointF(12, -20), "translation is independent of scale");
    backend.moveUnit(0, 1);
    backend.finishBackTranslation(movingId);
    check(preview.unitId.isNull(), "release clears wireframe");
    check(backend.units()[1].back->baseLayer != cached, "release rasterizes moved back after reorder");
    const auto committedOffset = backend.units()[1].back2FrontRelativePos;
    backend.beginBackTranslation(movingId);
    backend.translateBack(movingId, {5, 5});
    backend.cancelBackTranslation();
    check(backend.units()[1].back2FrontRelativePos == committedOffset && preview.unitId.isNull(), "cancel restores committed offset");
    check(!backend.beginBackTranslation(movable[1].id), "hidden back cannot be dragged");
    check(previews >= 5, "translation preview lifecycle");

    backend.setUnits({});
    check(!properties.hasSelection(), "replacement clears properties");
    qInfo("Layer model and back rendering tests passed");
}
