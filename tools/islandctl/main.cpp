// islandctl — command-line client for island-shell's D-Bus interface.

#include "controlinterface.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QTextStream>

#include <cstdio>

namespace {

constexpr auto kService = "org.islandshell.Shell";
constexpr auto kPath = "/org/islandshell/Shell";

QTextStream& out() {
    static QTextStream s(stdout);
    return s;
}

QTextStream& err() {
    static QTextStream s(stderr);
    return s;
}

int usage(int code) {
    (code == 0 ? out() : err()) << R"(usage: islandctl <command> [args]

commands:
  launcher toggle         open or close the launcher
  expand [activity]       expand the island (current activity if omitted)
  collapse                collapse the island
  power                   open the power menu
  dnd on|off|toggle       set do-not-disturb
  dnd                     print do-not-disturb state
  activities              list registered activities
  reload                  reload config, theme and user activities
  version                 print the shell version
)";
    return code;
}

int check(const QDBusPendingCall& call) {
    QDBusPendingCall c = call;
    c.waitForFinished();
    if (c.isError()) {
        const auto e = c.error();
        if (e.type() == QDBusError::ServiceUnknown)
            err() << "islandctl: island-shell is not running\n";
        else
            err() << "islandctl: " << e.message() << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("islandctl"));

    auto args = QCoreApplication::arguments().mid(1);
    if (args.isEmpty())
        return usage(2);
    const auto cmd = args.takeFirst();
    if (cmd == u"-h" || cmd == u"--help" || cmd == u"help")
        return usage(0);

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        err() << "islandctl: no D-Bus session bus\n";
        return 1;
    }
    OrgIslandshellControlInterface shell(QLatin1String(kService), QLatin1String(kPath), bus);

    if (cmd == u"launcher") {
        if (args.value(0, QStringLiteral("toggle")) != u"toggle")
            return usage(2);
        return check(shell.ToggleLauncher());
    }
    if (cmd == u"expand")
        return check(shell.Expand(args.value(0)));
    if (cmd == u"collapse")
        return check(shell.Collapse());
    if (cmd == u"power")
        return check(shell.ShowPower());
    if (cmd == u"reload")
        return check(shell.Reload());
    if (cmd == u"activities") {
        auto reply = shell.ListActivities();
        if (int rc = check(reply))
            return rc;
        for (const auto& name : reply.value())
            out() << name << '\n';
        return 0;
    }
    if (cmd == u"version") {
        if (!shell.isValid())
            return check(shell.Collapse()); // reports "not running"
        out() << shell.version() << '\n';
        return 0;
    }
    if (cmd == u"dnd") {
        const auto arg = args.value(0);
        if (arg.isEmpty() || arg == u"toggle") {
            if (!shell.isValid()) {
                err() << "islandctl: island-shell is not running\n";
                return 1;
            }
            const bool current = shell.dnd();
            if (arg.isEmpty()) {
                out() << (current ? "on" : "off") << '\n';
                return 0;
            }
            return check(shell.SetDnd(!current));
        }
        if (arg == u"on" || arg == u"off")
            return check(shell.SetDnd(arg == u"on"));
        return usage(2);
    }
    return usage(2);
}
