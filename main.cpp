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
    LayerUIModel layerModel;
    TextDepth backend(1920, 1080); // TODO: Unhardcode me
    TextDepthViewport frontend;

    engine.rootContext()->setContextProperty(
        "layerModel",
        &layerModel);
    engine.rootContext()->setContextProperty(
        "textDepthWidget",
        &frontend);

    engine.rootContext()->setContextProperty("textDepthBackend", &backend);

    QObject::connect(
        &layerModel,
        &LayerUIModel::addLayerSignal,
        &backend,
        &TextDepth::addLayer);

    QObject::connect(
        &backend,
        &TextDepth::notifyNewQtData,
        &frontend,
        &TextDepthViewport::handleNewQtData);

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
