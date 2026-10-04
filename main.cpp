#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQMLContext>
#include <TextDepth.h>
#include "TextDepthUnit.h"
#include "LayerUIModel.h"
#include <TextDepthViewport.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qRegisterMetaType<TextDepthUnits>("TextDepthUnits");

    QQmlApplicationEngine engine;
    TextDepth backend(1920, 1080); // TODO: Unhardcode me
    TextDepthViewport frontend;
    LayerUIModel layerModel;

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
