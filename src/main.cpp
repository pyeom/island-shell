#include "config.h"
#include "appprogress.h"
#include "downloadwatch.h"
#include "icons.h"
#include "keybinds.h"
#include "liveactivities.h"
#include "notifications.h"
#include "powermodes.h"
#include "shellcontrol.h"
#include "tray.h"
#include "updates.h"
#include "useractivities.h"

#include <QCommandLineParser>
#include <QSocketNotifier>
#include <QLocale>
#include <QTranslator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

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

// SIGTERM, SIGINT and SIGHUP quit through the event loop, so aboutToQuit
// handlers run (PowerModes puts Hyprland's options back). The handler only
// writes to a socket; the notifier does the rest on the main thread.
int g_signalPipe[2] = {-1, -1};
volatile std::sig_atomic_t g_signal = 0;

void onSignal(int sig) {
    g_signal = sig;
    const char c = 1;
    [[maybe_unused]] const auto n = ::write(g_signalPipe[0], &c, 1);
}

void quitOnSignals(QObject* parent) {
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, g_signalPipe) != 0)
        return;
    auto* notifier = new QSocketNotifier(g_signalPipe[1], QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated, parent, [] {
        char c = 0;
        [[maybe_unused]] const auto n = ::read(g_signalPipe[1], &c, 1);
        QCoreApplication::quit();
    });
    struct sigaction sa {};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    for (const int sig : {SIGTERM, SIGINT, SIGHUP})
        sigaction(sig, &sa, nullptr);
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
    quitOnSignals(&app);

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
        // Checks on its own schedule, not when quick settings first opens.
        island::Updates::instance();
        // Hyprland effects follow the power profile; one instance does it.
        island::PowerModes::instance();
        // Serves org.kde.StatusNotifierWatcher unless waybar or a panel does.
        island::Tray::instance()->start();
        break;
    }

    island::Icons::setupTheme(
        QString::fromStdString(island::Config::instance()->data().iconTheme));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new island::IconProvider);
    engine.addImageProvider(QStringLiteral("tray"), new island::TrayImageProvider);
    engine.setInitialProperties({{QStringLiteral("demo"), parser.isSet(demoOption)}});
    engine.loadFromModule("IslandShell.App", "Shell");
    if (engine.rootObjects().isEmpty())
        return 1;

    auto* userActivities = island::UserActivities::instance();
    userActivities->setEngine(&engine);
    userActivities->reload();

    const int status = QGuiApplication::exec();
    // Same exit status as dying from the signal: island-shell-run and
    // systemd tell a stop from a crash by it.
    return g_signal != 0 ? 128 + g_signal : status;
}
