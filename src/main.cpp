#include "config.h"
#include "appprogress.h"
#include "downloadwatch.h"
#include "icons.h"
#include "keybinds.h"
#include "liveactivities.h"
#include "notifications.h"
#include "shellcontrol.h"
#include "useractivities.h"

#include <QCommandLineParser>
#include <QLocale>
#include <QTranslator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(IslandShellPlugin)
Q_IMPORT_QML_PLUGIN(IslandShell_ActivitiesPlugin)
Q_IMPORT_QML_PLUGIN(IslandShell_AppPlugin)

namespace {

// --help and --version must work without a display (SSH, package checks), so
// they are answered before the GUI application exists.
bool wantsInfoOnly(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const QByteArrayView a(argv[i]);
        if (a == "--help" || a == "-h" || a == "--version" || a == "-v" || a == "--help-all")
            return true;
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    if (wantsInfoOnly(argc, argv)) {
        QCoreApplication info(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("island-shell"));
        QCoreApplication::setApplicationVersion(QStringLiteral(ISLAND_VERSION));
        QCommandLineParser parser;
        parser.setApplicationDescription(QStringLiteral("Dynamic Island style shell for Hyprland"));
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption({QStringLiteral("demo"), QStringLiteral("Cycle through island states for testing.")});
        parser.process(info); // prints and exits
        return 0;
    }

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("island-shell"));
    QGuiApplication::setApplicationVersion(QStringLiteral(ISLAND_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("island-shell"));

    // Embedded translations (":/i18n/island-shell_es.qm"); English otherwise.
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("island-shell"), QStringLiteral("_"),
                        QStringLiteral(":/i18n")))
        QCoreApplication::installTranslator(&translator);

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
        return 3; // island-shell-run and the systemd unit treat 3 as "do not restart"
    case island::ShellControl::Registration::NoBus:
        qWarning("island-shell: no D-Bus session bus; islandctl will not work");
        break;
    case island::ShellControl::Registration::Ok:
        island::Notifications::instance()->registerOnBus();
        island::LiveActivities::instance()->registerOnBus();
        // Only the instance that owns the bus: the binds call islandctl.
        island::Keybinds::instance();
        island::AppProgress::instance();
        island::DownloadWatch::instance();
        break;
    }

    island::Icons::setupTheme(
        QString::fromStdString(island::Config::instance()->data().iconTheme));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new island::IconProvider);
    engine.setInitialProperties({{QStringLiteral("demo"), parser.isSet(demoOption)}});
    engine.loadFromModule("IslandShell.App", "Shell");
    if (engine.rootObjects().isEmpty())
        return 1;

    auto* userActivities = island::UserActivities::instance();
    userActivities->setEngine(&engine);
    userActivities->reload();

    return QGuiApplication::exec();
}
