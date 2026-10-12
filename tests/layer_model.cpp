#include "LayerUIModel.h"
#include "TextDepth.h"
#include "UnitPropertiesModel.h"
#include <QGuiApplication>
#include "TextDepthViewport.h"
#include "qtToPhotoshopAPI.h"
#include <type_traits>
#include <QPersistentModelIndex>
#include <stdexcept>

static void check(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
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

    // Pixel-level stacking check through the actual viewport receiver and painter.
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
    TextDepthViewport viewport;
    QObject::connect(&backend, &TextDepth::notifyNewQtData, &viewport, &TextDepthViewport::handleNewQtData);
    const auto render = [&] {
        auto image = solid(Qt::transparent);
        QPainter painter(&image);
        viewport.paint(&painter);
        return image;
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
    backend.setUnits({});
    check(!properties.hasSelection(), "replacement clears properties");
    qInfo("Layer model and back rendering tests passed");
}
