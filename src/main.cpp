#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(IslandShellPlugin)

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("island-shell"));
    QGuiApplication::setApplicationVersion(QStringLiteral(ISLAND_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("island-shell"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Dynamic Island style shell for Hyprland"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption demoOption(
        QStringLiteral("demo"), QStringLiteral("Cycle through island states for testing."));
    parser.addOption(demoOption);
    parser.process(app);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("demo"), parser.isSet(demoOption)}});
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("IslandShell", "Shell");

    return QGuiApplication::exec();
}
