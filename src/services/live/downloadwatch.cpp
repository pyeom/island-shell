#include "downloadwatch.h"

#include "config.h"
#include "desktopentry.h"
#include "liveactivities.h"
#include "progressparse.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace island {

namespace {
constexpr int kShowAfterMs = 2000; // quick downloads stay quiet

QString activityId(const QString& target) {
    return u"download:"_s + target;
}
} // namespace

DownloadWatch* DownloadWatch::instance() {
    static auto* watch = new DownloadWatch(QCoreApplication::instance());
    return watch;
}

DownloadWatch::DownloadWatch(QObject* parent) : QObject(parent) {
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &DownloadWatch::tick);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &DownloadWatch::rescan);
    connect(Config::instance(), &Config::changed, this, &DownloadWatch::setup);
    setup();
}

QString DownloadWatch::directory() const {
    auto dir = QString::fromStdString(Config::instance()->data().live.downloadsDirectory);
    if (dir.startsWith(u"~/"_s))
        dir = QDir::homePath() + dir.mid(1);
    return dir.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : dir;
}

void DownloadWatch::setup() {
    const auto dir = Config::instance()->data().live.downloads ? directory() : QString();
    if (dir == m_dir)
        return;
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_dir = dir;
    m_active.clear();
    m_tick.stop();
    if (!m_dir.isEmpty() && QFileInfo(m_dir).isDir())
        m_watcher.addPath(m_dir);
    rescan();
}

void DownloadWatch::rescan() {
    if (m_dir.isEmpty())
        return;
    const auto partials = QDir(m_dir).entryList({u"*.part"_s, u"*.crdownload"_s}, QDir::Files | QDir::Hidden);
    auto* live = LiveActivities::instance();

    // Finished or cancelled.
    for (auto it = m_active.begin(); it != m_active.end();) {
        if (partials.contains(it.key())) {
            ++it;
            continue;
        }
        const auto& d = it.value();
        if (d.shown) {
            const auto path = QDir(m_dir).filePath(d.target);
            if (QFileInfo::exists(path)) {
                const auto actions = QStringList{u"open=%1"_s.arg(QCoreApplication::translate("DownloadWatch", "Open")),
                                                 u"folder=%1"_s.arg(QCoreApplication::translate("DownloadWatch", "Show in folder"))};
                live->set(activityId(d.target),
                          {{u"state"_s, u"success"_s},
                           {u"title"_s, d.target},
                           {u"subtitle"_s, QCoreApplication::translate("DownloadWatch", "Downloaded · %1")
                                               .arg(formatBytes(QFileInfo(path).size()))},
                           {u"actions"_s, actions},
                           {u"commands"_s, QVariantMap{{u"open"_s, u"xdg-open "_s + shellQuote(path)},
                                                       {u"folder"_s, u"xdg-open "_s + shellQuote(m_dir)}}},
                           {u"ttl"_s, 20}});
            } else if (live->contains(activityId(d.target))) {
                live->End(activityId(d.target));
            }
        }
        it = m_active.erase(it);
    }
    // New ones.
    const auto now = QDateTime::currentDateTime();
    for (const auto& name : partials) {
        const auto target = partialDownloadTarget(name);
        if (!target.isEmpty() && !m_active.contains(name))
            m_active.insert(name, {target, now, 0, false});
    }
    if (m_active.isEmpty())
        m_tick.stop();
    else if (!m_tick.isActive())
        m_tick.start();
}

void DownloadWatch::tick() {
    const auto now = QDateTime::currentDateTime();
    auto* live = LiveActivities::instance();
    for (auto it = m_active.begin(); it != m_active.end(); ++it) {
        auto& d = it.value();
        const qint64 size = QFileInfo(QDir(m_dir).filePath(it.key())).size();
        const qint64 speed = std::max<qint64>(0, size - d.size); // per second: the tick is 1 s
        d.size = size;
        if (d.seen.msecsTo(now) < kShowAfterMs)
            continue;
        auto subtitle = formatBytes(size);
        if (speed > 0)
            subtitle += u" · "_s + formatBytes(speed) + u"/s"_s;
        QVariantMap props{{u"subtitle"_s, subtitle}, {u"ttl"_s, 120}}; // refreshed every tick
        if (!d.shown) {
            props.insert(u"title"_s, d.target);
            props.insert(u"icon"_s, u"folder-download"_s);
            d.shown = true;
        }
        live->set(activityId(d.target), props);
    }
}

} // namespace island
