#include "shellcontrol.h"
#include "useractivities.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(IslandShellPlugin)
Q_IMPORT_QML_PLUGIN(IslandShell_ActivitiesPlugin)
Q_IMPORT_QML_PLUGIN(IslandShell_AppPlugin)

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

    switch (island::ShellControl::instance()->registerOnBus()) {
    case island::ShellControl::Registration::AlreadyRunning:
        qCritical("island-shell: another instance owns %s", island::ShellControl::kService);
        return 1;
    case island::ShellControl::Registration::NoBus:
        qWarning("island-shell: no D-Bus session bus; islandctl will not work");
        break;
    case island::ShellControl::Registration::Ok:
        break;
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("demo"), parser.isSet(demoOption)}});
    engine.loadFromModule("IslandShell.App", "Shell");
    if (engine.rootObjects().isEmpty())
        return 1;

    auto* userActivities = island::UserActivities::instance();
    userActivities->setEngine(&engine);
    userActivities->reload();

    return QGuiApplication::exec();
}
