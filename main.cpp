#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQMLContext>
 #include <textdepth.h>
#include "LayerUIModel.h"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    LayerUIModel layerModel;

    engine.rootContext()->setContextProperty(
        "layerModel",
        &layerModel);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("TextDepthOG", "Main");
    return app.exec();
}
