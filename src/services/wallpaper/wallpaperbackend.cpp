#include "wallpaperbackend.h"

#include <QDir>
#include <QFileInfo>

#include <array>
#include <utility>

namespace island::wallpaper {

namespace {
constexpr std::array<std::pair<Backend, QStringView>, 6> kNames{{
    {Backend::Awww, u"awww"},
    {Backend::Swww, u"swww"},
    {Backend::Hyprpaper, u"hyprpaper"},
    {Backend::Swaybg, u"swaybg"},
    {Backend::Waypaper, u"waypaper"},
    {Backend::Command, u"command"},
}};

QString canonical(QString path) {
    if (path.startsWith(QLatin1String("~/")))
        path = QDir::homePath() + path.mid(1);
    const auto resolved = QFileInfo(path).canonicalFilePath();
    return resolved.isEmpty() ? QDir::cleanPath(path) : resolved;
}

// "img %f" with the user's arguments in between: "img --transition-type grow %f".
QString withArgs(const QString& head, const QString& args, const QString& tail) {
    const auto extra = args.trimmed();
    return extra.isEmpty() ? head + u' ' + tail : head + u' ' + extra + u' ' + tail;
}
} // namespace

std::optional<Backend> backendFromName(QStringView name) {
    for (const auto& [backend, text] : kNames) {
        if (name == text)
            return backend;
    }
    return std::nullopt;
}

QString backendName(Backend backend) {
    for (const auto& [b, text] : kNames) {
        if (b == backend)
            return text.toString();
    }
    return QStringLiteral("none");
}

QString program(Backend backend) {
    switch (backend) {
    case Backend::Awww:
        return QStringLiteral("awww");
    case Backend::Swww:
        return QStringLiteral("swww");
    case Backend::Hyprpaper:
        return QStringLiteral("hyprpaper");
    case Backend::Swaybg:
        return QStringLiteral("swaybg");
    case Backend::Waypaper:
        return QStringLiteral("waypaper");
    case Backend::Command:
    case Backend::None:
        break;
    }
    return {};
}

QString daemonProcess(Backend backend) {
    switch (backend) {
    case Backend::Awww:
        return QStringLiteral("awww-daemon");
    case Backend::Swww:
        return QStringLiteral("swww-daemon");
    case Backend::Hyprpaper:
        return QStringLiteral("hyprpaper");
    case Backend::Swaybg:
        return QStringLiteral("swaybg");
    case Backend::Waypaper:
    case Backend::Command:
    case Backend::None:
        break;
    }
    return {};
}

QString applyCommand(Backend backend, const QString& args, const QString& custom) {
    switch (backend) {
    case Backend::Awww:
        return withArgs(QStringLiteral("awww img"), args, QStringLiteral("%f"));
    case Backend::Swww:
        return withArgs(QStringLiteral("swww img"), args, QStringLiteral("%f"));
    case Backend::Hyprpaper:
        // Older hyprpaper needs the preload; newer ones ignore it.
        return QStringLiteral("hyprctl hyprpaper preload %f; hyprctl hyprpaper wallpaper ,%f");
    case Backend::Swaybg:
        return withArgs(QStringLiteral("pkill -x swaybg; setsid -f swaybg -m fill"), args,
                        QStringLiteral("-i %f >/dev/null 2>&1"));
    case Backend::Waypaper:
        return withArgs(QStringLiteral("waypaper"), args, QStringLiteral("--wallpaper %f"));
    case Backend::Command:
        return custom;
    case Backend::None:
        break;
    }
    return {};
}

QStringList queryCommand(Backend backend) {
    switch (backend) {
    case Backend::Awww:
        return {QStringLiteral("awww"), QStringLiteral("query")};
    case Backend::Swww:
        return {QStringLiteral("swww"), QStringLiteral("query")};
    case Backend::Hyprpaper:
        return {QStringLiteral("hyprctl"), QStringLiteral("hyprpaper"), QStringLiteral("listactive")};
    case Backend::Swaybg:
    case Backend::Waypaper:
    case Backend::Command:
    case Backend::None:
        break;
    }
    return {};
}

QStringList parseQuery(QStringView text) {
    static constexpr QStringView image = u"currently displaying: image: ";
    QStringList shown;
    for (auto line : text.split(u'\n')) {
        line = line.trimmed();
        if (const auto at = line.indexOf(image); at >= 0) {
            shown.append(line.mid(at + image.size()).trimmed().toString());
        } else if (const auto eq = line.indexOf(u" = "); eq > 0 && !line.contains(u"currently displaying")) {
            const auto path = line.mid(eq + 3).trimmed();
            if (path.startsWith(u'/') || path.startsWith(u"~/"))
                shown.append(path.toString());
        }
    }
    return shown;
}

WaypaperState parseWaypaperIni(QStringView text) {
    WaypaperState state;
    for (auto line : text.split(u'\n')) {
        line = line.trimmed();
        const auto eq = line.indexOf(u'=');
        if (eq <= 0)
            continue;
        const auto key = line.left(eq).trimmed();
        const auto value = line.mid(eq + 1).trimmed();
        if (key == u"wallpaper")
            state.wallpaper = value.toString();
        else if (key == u"backend")
            state.backend = backendFromName(value);
    }
    return state;
}

QString swaybgImage(const QStringList& argv) {
    for (qsizetype i = 0; i < argv.size(); ++i) {
        const auto& a = argv.at(i);
        if ((a == u"-i" || a == u"--image") && i + 1 < argv.size())
            return argv.at(i + 1);
        if (a.startsWith(QLatin1String("--image=")))
            return a.mid(8);
    }
    return {};
}

QString match(const QStringList& files, const QStringList& shown) {
    for (const auto& s : shown) {
        const auto target = canonical(s);
        for (const auto& f : files) {
            if (canonical(f) == target)
                return f;
        }
    }
    return {};
}

} // namespace island::wallpaper
