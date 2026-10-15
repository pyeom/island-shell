#include "calendar.h"

#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJSEngine>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcCalendar, "island.calendar")

namespace island {

namespace {

QString expandHome(const std::string& path) {
    auto p = QString::fromStdString(path);
    if (p.startsWith(QLatin1String("~/")))
        p = QDir::homePath() + p.mid(1);
    return QDir::cleanPath(p);
}

QString readMeta(const QString& dir, const char* name) {
    QFile f(dir + u'/' + QLatin1String(name));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
}

} // namespace

Calendar* Calendar::create(QQmlEngine*, QJSEngine*) {
    static auto* cal = new Calendar(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(cal, QJSEngine::CppOwnership);
    return cal;
}

Calendar::Calendar(QObject* parent) : QObject(parent) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(1000);
    connect(&m_debounce, &QTimer::timeout, this, &Calendar::reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce,
            qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce,
            qOverload<>(&QTimer::start));
    connect(Config::instance(), &Config::changed, &m_debounce, qOverload<>(&QTimer::start));

    m_midnight.setSingleShot(true);
    connect(&m_midnight, &QTimer::timeout, this, &Calendar::reload);
    m_soonTimer.setInterval(30 * 1000);
    connect(&m_soonTimer, &QTimer::timeout, this, &Calendar::checkSoon);
    m_soonTimer.start();
    reload();
}

void Calendar::reload() {
    const auto& cfg = Config::instance()->data().calendar;
    const QDateTime from = QDate::currentDate().addDays(-40).startOfDay();
    const QDateTime to = QDate::currentDate().addDays(cfg.days + 1).startOfDay();

    QList<CalendarEvent> events;
    QStringList files;
    QStringList dirs;
    for (const auto& configured : cfg.paths) {
        const auto root = expandHome(configured);
        if (!QFileInfo(root).isDir())
            continue;
        dirs.append(root);
        QDirIterator it(root, {QStringLiteral("*.ics")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const auto path = it.next();
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                continue;
            files.append(path);
            // vdirsyncer: one collection per directory, with metadata files.
            const auto collection = QFileInfo(path).path();
            auto name = readMeta(collection, "displayname");
            if (name.isEmpty())
                name = QFileInfo(collection).fileName();
            const auto color = readMeta(collection, "color");
            for (auto& ev : expandEvents(f.readAll(), from, to)) {
                ev.calendar = name;
                ev.color = color;
                events.append(std::move(ev));
            }
            if (!dirs.contains(collection))
                dirs.append(collection);
        }
    }
    std::ranges::stable_sort(events, {}, &CalendarEvent::start);
    m_events = std::move(events);
    m_files = files;

    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!dirs.isEmpty())
        m_watcher.addPaths(dirs);

    qCInfo(lcCalendar) << "loaded" << m_events.size() << "events from" << m_files.size()
                       << "files";
    ++m_revision;
    scheduleMidnight();
    emit changed();
    checkSoon();
}

void Calendar::scheduleMidnight() {
    const auto now = QDateTime::currentDateTime();
    const auto next = QDate::currentDate().addDays(1).startOfDay().addSecs(5);
    m_midnight.start(static_cast<int>(std::min<qint64>(now.msecsTo(next), 24LL * 3600 * 1000)));
}

QVariantMap Calendar::toMap(const CalendarEvent& e) {
    return {
        {QStringLiteral("summary"), e.summary},
        {QStringLiteral("location"), e.location},
        {QStringLiteral("description"), e.description},
        {QStringLiteral("start"), e.start},
        {QStringLiteral("end"), e.end},
        {QStringLiteral("allDay"), e.allDay},
        {QStringLiteral("calendar"), e.calendar},
        {QStringLiteral("color"), e.color},
    };
}

QVariantList Calendar::eventsOn(const QDate& day) const {
    const auto start = day.startOfDay();
    const auto end = day.addDays(1).startOfDay();
    QVariantList out;
    for (const auto& e : m_events)
        if (e.start < end && e.end > start)
            out.append(toMap(e));
    return out;
}

QVariantList Calendar::upcoming(int limit) const {
    const auto now = QDateTime::currentDateTime();
    QVariantList out;
    for (const auto& e : m_events) {
        if (out.size() >= limit)
            break;
        if (e.end > now)
            out.append(toMap(e));
    }
    return out;
}

QList<int> Calendar::busyDays(int year, int month) const {
    const QDate first(year, month, 1);
    QList<int> days;
    for (const auto& e : m_events) {
        // Multi-day events mark every day they cover.
        for (QDate d = e.start.date(); d < e.end.date() || d == e.start.date(); d = d.addDays(1)) {
            if (d.year() == year && d.month() == month && !days.contains(d.day()))
                days.append(d.day());
            if (d > first.addMonths(1))
                break;
        }
    }
    std::ranges::sort(days);
    return days;
}

void Calendar::checkSoon() {
    const int minutes = Config::instance()->data().calendar.reminderMinutes;
    const auto now = QDateTime::currentDateTime();
    QVariantMap soon;
    if (minutes > 0) {
        for (const auto& e : m_events) {
            if (e.allDay)
                continue;
            // From `minutes` before start until it has been running for 5 minutes.
            if (e.start > now.addSecs(minutes * 60LL))
                break;
            if (e.start.addSecs(5 * 60) > now) {
                soon = toMap(e);
                break;
            }
        }
    }
    if (soon != m_soon) {
        m_soon = soon;
        emit soonChanged();
    }
}

} // namespace island
