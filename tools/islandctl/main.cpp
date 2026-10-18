// islandctl — command-line client for island-shell's D-Bus interface.

#include "controlinterface.h"
#include "historyinterface.h"
#include "liveinterface.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QTextStream>

#include <cstdio>
#include <optional>

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
  expand [activity]       expand the island (what a click opens if omitted)
  collapse                collapse the island
  power                   open the power menu
  quick                   open quick settings
  settings                open the settings window
  screenshot MODE         region | window | output | active
  wallpaper [random]      open the wallpaper picker, or apply a random one
  media ACTION            play-pause | next | previous, on the player the island shows
  volume ACTION           up | down (5%) | +N | -N | N (percent) | mute | mic-mute
  dnd on|off|toggle       set do-not-disturb
  dnd                     print do-not-disturb state
  activities              list registered activities
  reload                  reload config, theme and user activities
  version                 print the shell version

live activities:
  push --id ID [options] [TITLE]    create or replace a live activity
  update --id ID [options] [TITLE]  change some of its fields
  end ID                            remove it
  live                              list live activities
    --title T  --subtitle S  --icon NAME|PATH
    --progress 0..1   --state running|success|error
    --ttl SECONDS (default 1800 for push, 0 = never)
    --action "Label=shell command"   (repeatable)

notifications:
  notifications [list]    print history, newest first
  notifications clear     dismiss everything
  notifications dismiss ID
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

// Parses live activity options into D-Bus props. Returns false on bad input.
bool parseLiveArgs(QStringList args, QString& id, QVariantMap& props) {
    QStringList actions;
    QVariantMap commands;
    QStringList titleWords;
    while (!args.isEmpty()) {
        const auto arg = args.takeFirst();
        auto value = [&](const char* name) -> std::optional<QString> {
            if (args.isEmpty()) {
                err() << "islandctl: " << name << " needs a value\n";
                return std::nullopt;
            }
            return args.takeFirst();
        };
        if (arg == u"--id") {
            const auto v = value("--id");
            if (!v)
                return false;
            id = *v;
        } else if (arg == u"--title" || arg == u"--subtitle" || arg == u"--icon" ||
                   arg == u"--state") {
            const auto v = value(qPrintable(arg));
            if (!v)
                return false;
            props.insert(arg.mid(2), *v);
        } else if (arg == u"--progress") {
            const auto v = value("--progress");
            bool ok = false;
            const double p = v ? v->toDouble(&ok) : 0;
            if (!ok) {
                err() << "islandctl: --progress expects a number between 0 and 1\n";
                return false;
            }
            props.insert(QStringLiteral("progress"), p);
        } else if (arg == u"--ttl") {
            const auto v = value("--ttl");
            bool ok = false;
            const int ttl = v ? v->toInt(&ok) : 0;
            if (!ok) {
                err() << "islandctl: --ttl expects seconds\n";
                return false;
            }
            props.insert(QStringLiteral("ttl"), ttl);
        } else if (arg == u"--action") {
            const auto v = value("--action");
            const auto eq = v ? v->indexOf(u'=') : -1;
            if (eq <= 0) {
                err() << "islandctl: --action expects \"Label=command\"\n";
                return false;
            }
            const auto key = QStringLiteral("a%1").arg(actions.size());
            actions.append(key + u'=' + v->left(eq));
            commands.insert(key, v->mid(eq + 1));
        } else if (arg.startsWith(QLatin1String("--"))) {
            err() << "islandctl: unknown option " << arg << '\n';
            return false;
        } else {
            titleWords.append(arg);
        }
    }
    if (!titleWords.isEmpty())
        props.insert(QStringLiteral("title"), titleWords.join(u' '));
    if (!actions.isEmpty()) {
        props.insert(QStringLiteral("actions"), actions);
        props.insert(QStringLiteral("commands"), commands);
    }
    if (id.isEmpty()) {
        err() << "islandctl: --id is required\n";
        return false;
    }
    return true;
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

    if (cmd == u"push" || cmd == u"update") {
        QString id;
        QVariantMap props;
        if (!parseLiveArgs(args, id, props))
            return 2;
        OrgIslandshellLiveActivitiesInterface live(
            QLatin1String(kService), QStringLiteral("/org/islandshell/LiveActivities"), bus);
        if (cmd == u"update")
            return check(live.Update(id, props));
        // islandctl exits right away, so the activity must outlive it.
        props.insert(QStringLiteral("persist"), true);
        return check(live.Push(id, props));
    }
    if (cmd == u"end" || cmd == u"live") {
        OrgIslandshellLiveActivitiesInterface live(
            QLatin1String(kService), QStringLiteral("/org/islandshell/LiveActivities"), bus);
        if (cmd == u"end") {
            if (args.isEmpty())
                return usage(2);
            return check(live.End(args.first()));
        }
        auto reply = live.List();
        if (int rc = check(reply))
            return rc;
        for (const auto& id : reply.value())
            out() << id << '\n';
        return 0;
    }
    if (cmd == u"notifications") {
        qDBusRegisterMetaType<QList<QVariantMap>>();
        OrgIslandshellNotificationsHistoryInterface history(
            QLatin1String(kService), QStringLiteral("/org/islandshell/Notifications"), bus);
        const auto sub = args.value(0, QStringLiteral("list"));
        if (sub == u"clear")
            return check(history.Clear());
        if (sub == u"dismiss") {
            bool ok = false;
            const uint id = args.value(1).toUInt(&ok);
            return ok ? check(history.Dismiss(id)) : usage(2);
        }
        if (sub != u"list")
            return usage(2);
        auto reply = history.List();
        if (int rc = check(reply))
            return rc;
        for (const auto& n : reply.value()) {
            out() << n.value(QStringLiteral("id")).toUInt() << '\t'
                  << n.value(QStringLiteral("time")).toString() << '\t'
                  << n.value(QStringLiteral("app")).toString() << '\t'
                  << n.value(QStringLiteral("summary")).toString() << '\n';
        }
        return 0;
    }

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
    if (cmd == u"quick")
        return check(shell.ShowQuickSettings());
    if (cmd == u"settings")
        return check(shell.ShowSettings());
    if (cmd == u"wallpaper")
        return check(shell.Wallpaper(args.value(0, QStringLiteral("pick"))));
    if (cmd == u"screenshot")
        return args.isEmpty() ? usage(2) : check(shell.Screenshot(args.first()));
    if (cmd == u"reload")
        return check(shell.Reload());
    if (cmd == u"media")
        return args.isEmpty() ? usage(2) : check(shell.Media(args.first()));
    if (cmd == u"volume")
        return args.isEmpty() ? usage(2) : check(shell.Volume(args.first()));
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
