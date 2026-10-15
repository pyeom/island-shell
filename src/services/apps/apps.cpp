#include "apps.h"

#include "config.h"
#include "hyprland.h"
#include "icons.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>

Q_LOGGING_CATEGORY(lcApps, "island.apps")

namespace island {

namespace {

QString usagePath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
           QLatin1String("/island-shell/launcher.json");
}

QStringList applicationDirs() {
    return QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                     QStringLiteral("applications"),
                                     QStandardPaths::LocateDirectory);
}

bool tryExecOk(const QString& tryExec) {
    if (tryExec.isEmpty())
        return true;
    if (tryExec.startsWith(u'/'))
        return QFileInfo(tryExec).isExecutable();
    return !QStandardPaths::findExecutable(tryExec).isEmpty();
}

} // namespace

Apps* Apps::instance() {
    static auto* apps = new Apps(QCoreApplication::instance());
    return apps;
}

Apps* Apps::create(QQmlEngine*, QJSEngine*) {
    auto* apps = instance();
    QJSEngine::setObjectOwnership(apps, QJSEngine::CppOwnership);
    return apps;
}

Apps::Apps(QObject* parent) : QObject(parent) {
    m_rescan.setSingleShot(true);
    m_rescan.setInterval(500);
    connect(&m_rescan, &QTimer::timeout, this, &Apps::scan);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_rescan,
            qOverload<>(&QTimer::start));
    loadUsage();
    scan();
}

void Apps::scan() {
    const auto locale = QLocale::system().name();
    const auto desktops =
        qEnvironmentVariable("XDG_CURRENT_DESKTOP", QStringLiteral("Hyprland")).split(u':');
    QSet<QString> seen;
    QList<DesktopEntry> entries;
    const auto dirs = applicationDirs();
    // Earlier directories take precedence ($XDG_DATA_HOME first).
    for (const auto& dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const auto path = it.next();
            // Desktop file ID: path relative to the dir, '/' replaced by '-'.
            auto id = QDir(dir).relativeFilePath(path).replace(u'/', u'-');
            if (seen.contains(id))
                continue;
            seen.insert(id);
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                continue;
            auto entry = parseDesktopEntry(QString::fromUtf8(f.readAll()), locale);
            if (!entry || !entry->visibleIn(desktops) || !tryExecOk(entry->tryExec))
                continue;
            entry->id = id;
            entries.append(std::move(*entry));
        }
    }
    std::ranges::sort(entries, [](const DesktopEntry& a, const DesktopEntry& b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });
    m_entries = std::move(entries);

    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!dirs.isEmpty())
        m_watcher.addPaths(dirs);
    qCInfo(lcApps) << "found" << m_entries.size() << "applications";
    emit changed();
}

QVariantList Apps::search(const QString& query, int limit) const {
    const auto q = query.trimmed();
    struct Hit {
        int score;
        const DesktopEntry* entry;
    };
    std::vector<Hit> hits;
    for (const auto& e : m_entries) {
        const int score = rankEntry(e, q, m_usage.value(e.id));
        if (score >= 0)
            hits.push_back({score, &e});
    }
    // Stable: ties keep alphabetical order.
    std::ranges::stable_sort(hits, std::greater{}, &Hit::score);

    QVariantList out;
    for (const auto& h : hits) {
        if (out.size() >= limit)
            break;
        QVariantList actions;
        for (const auto& a : h.entry->actions)
            actions.append(QVariantMap{{QStringLiteral("id"), a.id},
                                       {QStringLiteral("name"), a.name},
                                       {QStringLiteral("icon"), Icons::source(a.icon)}});
        out.append(QVariantMap{
            {QStringLiteral("id"), h.entry->id},
            {QStringLiteral("name"), h.entry->name},
            {QStringLiteral("genericName"), h.entry->genericName},
            {QStringLiteral("comment"), h.entry->comment},
            {QStringLiteral("icon"),
             Icons::source(h.entry->icon, QStringLiteral("application-x-executable"))},
            {QStringLiteral("actions"), actions},
        });
    }
    return out;
}

QString Apps::terminal() const {
    const auto& configured = Config::instance()->data().launcher.terminal;
    if (!configured.empty())
        return QString::fromStdString(configured);
    const auto env = qEnvironmentVariable("TERMINAL");
    if (!env.isEmpty())
        return env;
    for (const auto* t : {"kitty", "alacritty", "foot", "wezterm", "konsole"})
        if (!QStandardPaths::findExecutable(QLatin1String(t)).isEmpty())
            return QLatin1String(t);
    return QStringLiteral("xterm");
}

bool Apps::launch(const QString& id, const QString& action) {
    const auto it = std::ranges::find(m_entries, id, &DesktopEntry::id);
    if (it == m_entries.end())
        return false;
    QString exec = it->exec;
    if (!action.isEmpty()) {
        const auto a = std::ranges::find(it->actions, action, &DesktopAction::id);
        if (a == it->actions.end())
            return false;
        exec = a->exec;
    }
    auto command = stripFieldCodes(exec);
    if (it->terminal)
        command = terminal() + QLatin1String(" -e ") + command;
    run(command);

    m_usage[id] += 1;
    saveUsage();
    return true;
}

void Apps::run(const QString& command) {
    auto full = command;
    const auto prefix = QString::fromStdString(Config::instance()->data().launcher.launchPrefix);
    if (!prefix.isEmpty())
        full = prefix + u' ' + full;
    qCInfo(lcApps).noquote() << "launch:" << full;
    // Through Hyprland so the app gets the compositor's environment and
    // window rules; otherwise detach it into its own session.
    if (Hyprland::instance()->available()) {
        Hyprland::instance()->dispatch(QStringLiteral("exec ") + full);
        return;
    }
    QProcess p;
    p.setProgram(QStringLiteral("sh"));
    p.setArguments({QStringLiteral("-c"), full});
    p.setUnixProcessParameters(QProcess::UnixProcessFlag::CreateNewSession |
                               QProcess::UnixProcessFlag::CloseFileDescriptors);
    p.startDetached();
}

void Apps::loadUsage() {
    QFile f(usagePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const auto obj = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        m_usage.insert(it.key(), it.value().toInt());
}

void Apps::saveUsage() {
    QJsonObject obj;
    for (auto it = m_usage.cbegin(); it != m_usage.cend(); ++it)
        obj.insert(it.key(), it.value());
    QDir().mkpath(QFileInfo(usagePath()).path());
    QSaveFile f(usagePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        f.commit();
    }
}

} // namespace island
