#include "calendar.h"

#include "config.h"
#include "quickevent.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QUuid>

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

    // Collections: folders with a displayname or .ics files (vdir layout),
    // including empty ones vdirsyncer just created.
    QVariantList collections;
    for (const auto& configured : cfg.paths) {
        const auto root = expandHome(configured);
        QStringList candidates{root};
        for (const auto& sub : QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
            candidates.append(root + u'/' + sub);
        for (const auto& dir : candidates) {
            const QDir d(dir);
            if (!d.exists() || (!QFile::exists(dir + QLatin1String("/displayname")) &&
                                d.entryList({QStringLiteral("*.ics")}, QDir::Files).isEmpty()))
                continue;
            auto name = readMeta(dir, "displayname");
            if (name.isEmpty())
                name = d.dirName();
            collections.append(QVariantMap{{QStringLiteral("name"), name},
                                           {QStringLiteral("path"), dir},
                                           {QStringLiteral("color"), readMeta(dir, "color")}});
        }
    }
    const auto wanted = QString::fromStdString(cfg.defaultCalendar);
    std::ranges::stable_partition(collections, [&](const QVariant& c) {
        const auto m = c.toMap();
        return !wanted.isEmpty() && (m.value(QStringLiteral("name")) == wanted ||
                                     QFileInfo(m.value(QStringLiteral("path")).toString()).fileName() == wanted);
    });
    m_collections = collections;
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

QString Calendar::defaultCollection() const {
    if (!m_collections.isEmpty())
        return m_collections.first().toMap().value(QStringLiteral("path")).toString();
    // No calendars yet: a local one, created with the first event.
    const auto& paths = Config::instance()->data().calendar.paths;
    return paths.empty() ? QString() : expandHome(paths.back()) + QLatin1String("/local");
}

QVariantMap Calendar::previewQuick(const QString& text) const {
    QString error;
    const auto e = parseQuickEvent(text, QDateTime::currentDateTime(), &error);
    if (!e)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    if (defaultCollection().isEmpty())
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), QStringLiteral("no calendar folders in [calendar] paths")}};
    return {
        {QStringLiteral("ok"), true},
        {QStringLiteral("summary"), e->summary},
        {QStringLiteral("start"), e->start},
        {QStringLiteral("end"), e->end},
        {QStringLiteral("allDay"), e->allDay},
        {QStringLiteral("calendar"), m_collections.isEmpty()
                                         ? QVariant(QStringLiteral("Local"))
                                         : m_collections.first().toMap().value(QStringLiteral("name"))},
    };
}

bool Calendar::addQuick(const QString& text) {
    const auto e = parseQuickEvent(text, QDateTime::currentDateTime());
    return e && addEvent(e->summary, e->start, e->end, e->allDay);
}

bool Calendar::addEvent(const QString& summary, const QDateTime& start, const QDateTime& end,
                        bool allDay, const QString& collectionPath) {
    const auto dir = collectionPath.isEmpty() ? defaultCollection() : collectionPath;
    if (dir.isEmpty() || summary.trimmed().isEmpty() || !start.isValid() || end < start)
        return false;
    const auto uid = QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1String("@island-shell");
    QuickEvent e{summary.trimmed(), start, end.isValid() ? end : start.addSecs(3600), allDay};
    if (allDay) {
        e.start = start.date().startOfDay();
        e.end = std::max(end.date(), start.date().addDays(1)).startOfDay();
    }
    if (!QFileInfo::exists(dir)) {
        if (!QDir().mkpath(dir))
            return false;
        QFile name(dir + QLatin1String("/displayname"));
        if (name.open(QIODevice::WriteOnly))
            name.write("Local\n");
    }
    // vdir: one event per file, named after its UID.
    QFile f(dir + u'/' + uid.section(u'@', 0, 0) + QLatin1String(".ics"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        return false;
    f.write(eventToIcs(e, uid, QDateTime::currentDateTimeUtc()));
    f.close();
    qCInfo(lcCalendar).noquote() << "added" << e.summary << "to" << dir;
    reload();
    return true;
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
