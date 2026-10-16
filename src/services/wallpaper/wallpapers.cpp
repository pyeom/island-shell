#include "wallpapers.h"

#include "apps.h"
#include "config.h"
#include "desktopentry.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace island {

using wallpaper::Backend;

namespace {
bool has(const QString& program) {
    return !program.isEmpty() && !QStandardPaths::findExecutable(program).isEmpty();
}

QString readFile(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

// Command line of a running process called `name` (as in /proc/PID/comm),
// empty when there is none.
QStringList processArgs(const QString& name) {
    if (name.isEmpty())
        return {};
    const QDir proc(QStringLiteral("/proc"));
    for (const auto& pid : proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!pid.front().isDigit() || readFile(proc.filePath(pid + QLatin1String("/comm"))).trimmed() != name)
            continue;
        auto args = readFile(proc.filePath(pid + QLatin1String("/cmdline"))).split(QChar(0), Qt::SkipEmptyParts);
        return args.isEmpty() ? QStringList{name} : args;
    }
    return {};
}
} // namespace

Wallpapers* Wallpapers::create(QQmlEngine*, QJSEngine*) {
    static auto* w = new Wallpapers(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(w, QJSEngine::CppOwnership);
    return w;
}

Wallpapers::Wallpapers(QObject* parent) : QObject(parent) {
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Wallpapers::scan);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Wallpapers::scan);
    connect(Config::instance(), &Config::changed, this, &Wallpapers::scan);
    scan();
}

QString Wallpapers::directory() const {
    auto dir = QString::fromStdString(Config::instance()->data().wallpaper.directory);
    if (dir.startsWith(QLatin1String("~/")))
        dir = QDir::homePath() + dir.mid(1);
    return dir;
}

QString Wallpapers::statePath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
           QLatin1String("/island-shell/wallpaper");
}

QString Wallpapers::walCachePath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/wal/wal");
}

QString Wallpapers::waypaperIniPath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
           QLatin1String("/waypaper/config.ini");
}

void Wallpapers::scan() {
    const QDir dir(directory());
    QStringList files;
    for (const auto& info : dir.entryInfoList({QStringLiteral("*.png"), QStringLiteral("*.jpg"),
                                               QStringLiteral("*.jpeg"), QStringLiteral("*.webp"),
                                               QStringLiteral("*.gif")},
                                              QDir::Files, QDir::Name | QDir::IgnoreCase))
        files.append(info.absoluteFilePath());

    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (dir.exists())
        m_watcher.addPath(dir.absolutePath());
    for (const auto& path : {walCachePath(), waypaperIniPath()}) {
        if (QFile::exists(path))
            m_watcher.addPath(path);
    }

    const auto backend = resolveBackend();
    if (files != m_files || backend != m_backend) {
        m_files = files;
        m_backend = backend;
        emit changed();
    }
    resolveCurrent();
    queryBackend();
}

Backend Wallpapers::resolveBackend() const {
    const auto& cfg = Config::instance()->data().wallpaper;
    const auto name = QString::fromStdString(cfg.backend);
    if (name == u"auto" && !cfg.apply.empty())
        return Backend::Command;
    if (const auto b = wallpaper::backendFromName(name))
        return *b;

    if (has(QStringLiteral("waypaper")) && QFile::exists(waypaperIniPath()))
        return Backend::Waypaper;
    static constexpr std::array daemons{Backend::Awww, Backend::Swww, Backend::Hyprpaper, Backend::Swaybg};
    for (const auto b : daemons) {
        if (!processArgs(wallpaper::daemonProcess(b)).isEmpty())
            return b;
    }
    for (const auto b : daemons) {
        if (has(wallpaper::program(b)))
            return b;
    }
    return has(QStringLiteral("waypaper")) ? Backend::Waypaper : Backend::None;
}

void Wallpapers::queryBackend() {
    if (m_query) {
        m_queryAgain = true; // the running query may predate the change
        return;
    }
    // waypaper drives another backend; ask that one.
    auto target = m_backend;
    if (target == Backend::Waypaper)
        target = wallpaper::parseWaypaperIni(readFile(waypaperIniPath())).backend.value_or(Backend::None);

    if (target == Backend::Swaybg) {
        const auto image = wallpaper::swaybgImage(processArgs(QStringLiteral("swaybg")));
        m_shown = image.isEmpty() ? QStringList() : QStringList{image};
        resolveCurrent();
        return;
    }
    const auto command = wallpaper::queryCommand(target);
    if (command.isEmpty() || !has(command.front())) {
        m_shown.clear();
        resolveCurrent();
        return;
    }
    m_query = new QProcess(this);
    auto* p = m_query.data();
    connect(p, &QProcess::finished, this, [this, p](int code, QProcess::ExitStatus status) {
        // No daemon running: the query fails and the fallbacks decide.
        m_shown = code == 0 && status == QProcess::NormalExit
                      ? wallpaper::parseQuery(QString::fromUtf8(p->readAllStandardOutput()))
                      : QStringList();
        queryDone(p);
    });
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        m_shown.clear();
        queryDone(p);
    });
    p->start(command.front(), command.mid(1));
}

void Wallpapers::queryDone(QProcess* process) {
    process->deleteLater();
    m_query = nullptr;
    resolveCurrent();
    if (std::exchange(m_queryAgain, false))
        queryBackend();
}

void Wallpapers::resolveCurrent() {
    auto current = wallpaper::match(m_files, m_shown);
    if (current.isEmpty()) {
        // Newest first among pywal's cache, waypaper's choice and the island's own.
        struct Source {
            QDateTime modified;
            QString path;
        };
        std::vector<Source> sources;
        const auto add = [&](const QString& file, const QString& path) {
            if (!path.isEmpty())
                sources.push_back({QFileInfo(file).lastModified(), path});
        };
        add(statePath(), readFile(statePath()).trimmed());
        add(walCachePath(), readFile(walCachePath()).trimmed());
        if (m_backend == Backend::Waypaper)
            add(waypaperIniPath(), wallpaper::parseWaypaperIni(readFile(waypaperIniPath())).wallpaper);
        std::ranges::stable_sort(sources, std::greater{}, &Source::modified);
        QStringList candidates;
        for (const auto& s : sources)
            candidates << s.path;
        current = wallpaper::match(m_files, candidates);
    }
    if (current != m_current) {
        m_current = current;
        emit changed();
    }
}

void Wallpapers::apply(const QString& file) {
    if (!m_files.contains(file))
        return;
    const auto& cfg = Config::instance()->data().wallpaper;
    auto command = wallpaper::applyCommand(m_backend, QString::fromStdString(cfg.backendArgs),
                                           QString::fromStdString(cfg.apply));
    if (command.isEmpty())
        return;
    // A custom command runs wal itself if it wants to.
    if (cfg.pywal && m_backend != Backend::Command && has(QStringLiteral("wal")))
        command = u'(' + command + QLatin1String(") && wal -q -n -i %f");
    command.replace(QStringLiteral("%f"), shellQuote(file));
    Apps::instance()->run(command);

    // Show the choice right away; the backend may still report the old image
    // until the command has run, so ask it again a little later.
    QDir().mkpath(QFileInfo(statePath()).absolutePath());
    QFile state(statePath());
    if (state.open(QIODevice::WriteOnly | QIODevice::Truncate))
        state.write(file.toUtf8() + '\n');
    state.close();
    m_shown.clear();
    resolveCurrent();
    QTimer::singleShot(1500, this, &Wallpapers::queryBackend);
}

void Wallpapers::random() {
    QStringList others = m_files;
    others.removeAll(m_current);
    if (others.isEmpty())
        others = m_files;
    if (!others.isEmpty())
        apply(others.at(static_cast<qsizetype>(QRandomGenerator::global()->bounded(others.size()))));
}

} // namespace island
