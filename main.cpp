#include <QGuiApplication>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include <QQMLContext>
#include <TextDepth.h>
#include "TextDepthUnit.h"
#include "LayerUIModel.h"
#include "UnitPropertiesModel.h"
#include <TextDepthViewport.h>

int main(int argc, char *argv[])
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QSurfaceFormat format;
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    QGuiApplication app(argc, argv);
    qRegisterMetaType<RenderedTextDepthUnits>("RenderedTextDepthUnits");

    QQmlApplicationEngine engine;
    TextDepth backend(1920, 1080); // TODO: Unhardcode me
    TextDepthViewport frontend;
    LayerUIModel layerModel;
    UnitPropertiesModel unitProperties;

    engine.rootContext()->setContextProperty(
        "documentLayerModel",
        &layerModel);
    engine.rootContext()->setContextProperty(
        "textDepthWidget",
        &frontend);

    engine.rootContext()->setContextProperty("textDepthBackend", &backend);

    QObject::connect(
        &backend,
        &TextDepth::notifyNewQtData,
        &frontend,
        &TextDepthViewport::handleNewQtData);

    // Both views independently observe the authoritative document state.
    QObject::connect(&backend, &TextDepth::notifyNewQtData,
                     &layerModel, &LayerUIModel::setUnits);
    QObject::connect(&layerModel, &LayerUIModel::moveRequested,
                     &backend, &TextDepth::moveUnit);

    engine.rootContext()->setContextProperty("unitPropertiesModel", &unitProperties);
    QObject::connect(&backend, &TextDepth::notifyNewQtData,
                     &unitProperties, &UnitPropertiesModel::setUnits);
    QObject::connect(&layerModel, &LayerUIModel::selectedRowChanged, &unitProperties, [&] {
        unitProperties.selectUnit(layerModel.selectedUnitId());
    });
    QObject::connect(&unitProperties, &UnitPropertiesModel::toggleBackRequested,
                     &backend, &TextDepth::handleToggleUnitVisibility);

    QObject::connect(&unitProperties, &UnitPropertiesModel::shadowGradientRequested, &backend, &TextDepth::handleSetUnitShadowGradient);
    QObject::connect(&unitProperties, &UnitPropertiesModel::translationStarted, &backend, &TextDepth::beginBackTranslation);
    QObject::connect(&unitProperties, &UnitPropertiesModel::translationChanged, &backend, &TextDepth::translateBack);
    QObject::connect(&unitProperties, &UnitPropertiesModel::translationFinished, &backend, &TextDepth::finishBackTranslation);
    QObject::connect(&unitProperties, &UnitPropertiesModel::translationCanceled, &backend, &TextDepth::cancelBackTranslation);
    QObject::connect(&backend, &TextDepth::backPreviewChanged, &frontend, &TextDepthViewport::handleBackPreview);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []()
        { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("TextDepthOG", "Main");
    return app.exec();
}
