#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QString zipFilePath;
    if (argc > 1) {
        zipFilePath = QString::fromLocal8Bit(argv[1]);
    }

    engine.rootContext()->setContextProperty("initialZipPath", zipFilePath);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    if (zipFilePath.isEmpty()) {
        engine.loadFromModule("ZipExtract", "Main");
    } else {
        engine.loadFromModule("ZipExtract", "Extractor");
    }

    return app.exec();
}
