#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQMLContext>
 #include <textdepth.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("TextDepthOG", "Main");

    return app.exec();
}
